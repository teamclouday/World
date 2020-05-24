#include "data.hpp"
#include "files.hpp"

#include "global.hpp"
extern Application* app;

#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace DATA;

Graph::Graph(std::vector<MeshInput>& meshes, VkDevice backendDevice)
{
    d_device = backendDevice;
    convertInputMeshes(meshes);
    createIndiceBuffers();
    createVertexBuffers();
    createUniformBuffers();
    createDescriptorSets();
}

Graph::~Graph()
{
    app->GetRenderer()->freeRenderCommandBuffers(d_commands);
	// TODO: Free texture resources
	for(size_t i = 0; i < d_ubo_buffers.size(); i++)
        d_ubo_buffers[i].destroy(d_device);
    for(size_t i = 0; i < d_indice_buffers.size(); i++)
        d_indice_buffers[i].destroy(d_device);
    for(size_t i = 0; i < d_vertex_buffers.size(); i++)
        d_vertex_buffers[i].destroy(d_device);
    d_desctiptor_sets.destroy(d_device);
    for(size_t i = 0; i < d_meshes.size(); i++)
        d_meshes[i].destroy(d_device);
    d_device = VK_NULL_HANDLE;
}

void Graph::convertInputMeshes(std::vector<MeshInput>& meshes)
{
	LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_GRAPH;

    d_meshes.resize(meshes.size());
    d_ubo_per_mesh.resize(meshes.size());

	std::set<std::string> uniqueTexturePaths;
	for(const auto& mesh : meshes)
		uniqueTexturePaths.insert(mesh.textureImagePath);
	createTextures(uniqueTexturePaths);

    for(size_t i = 0; i < meshes.size(); i++)
    {
        Mesh& newMesh = d_meshes[i];
		newMesh.vertices.resize(meshes[i].vertices.size());
        memcpy(newMesh.vertices.data(), meshes[i].vertices.data(), sizeof(Vertex) * meshes[i].vertices.size());
		newMesh.indices.resize(meshes[i].indices.size());
        memcpy(newMesh.indices.data(), meshes[i].indices.data(), sizeof(uint32_t) * meshes[i].indices.size());
		if(meshes[i].textureImagePath != "")
        	newMesh.texture = d_unique_textures[meshes[i].textureImagePath];
        newMesh.allset = true;
        d_ubo_per_mesh[i].model = glm::mat4(1.0f);
    }

	if(myLogger){myLogger->AddMessage(myLoggerOwner, "graph meshes converted");}
}

void Graph::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(CameraUniform);
	
    d_ubo_buffers.resize(d_meshes.size());
    for(size_t i = 0; i < d_meshes.size(); i++)
    {
        d_ubo_buffers[i] = createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

void Graph::createDescriptorSets()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_GRAPH;

	d_desctiptor_sets.layout.resize(d_meshes.size());
	for(size_t i = 0; i < d_meshes.size(); i++)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings{};

    	VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(uboLayoutBinding);

		if(d_meshes[i].texture.allset)
		{
			VkDescriptorSetLayoutBinding samplerLayoutBinding{};
			samplerLayoutBinding.binding = 1;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.pImmutableSamplers = nullptr;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings.push_back(samplerLayoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(d_device, &layoutInfo, nullptr, &d_desctiptor_sets.layout[i]) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan descriptor set layout!");
	}

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan descriptor set layout created");}

    std::array<VkDescriptorPoolSize, 2> poolSize{};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = static_cast<uint32_t>(d_meshes.size());
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = static_cast<uint32_t>(d_meshes.size());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	poolInfo.pPoolSizes = poolSize.data();
	poolInfo.maxSets = static_cast<uint32_t>(d_meshes.size());

	if (vkCreateDescriptorPool(d_device, &poolInfo, nullptr, &d_desctiptor_sets.pool) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan descriptor pool!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan descriptor pool created");}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = d_desctiptor_sets.pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(d_meshes.size());
	allocInfo.pSetLayouts = d_desctiptor_sets.layout.data();

    d_desctiptor_sets.sets.resize(d_meshes.size());
	if (vkAllocateDescriptorSets(d_device, &allocInfo, d_desctiptor_sets.sets.data()) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to allocate Vulkan descriptor sets!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan descriptor sets allocated");}

    for(size_t i = 0; i < d_meshes.size(); i++)
    {
		std::vector<VkWriteDescriptorSet> descriptorWrite;

        VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = d_ubo_buffers[i].buf;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(CameraUniform);

		VkWriteDescriptorSet writeUniform;
		writeUniform.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeUniform.dstSet = d_desctiptor_sets.sets[i];
		writeUniform.dstBinding = 0;
		writeUniform.dstArrayElement = 0;
		writeUniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeUniform.descriptorCount = 1;
		writeUniform.pBufferInfo = &bufferInfo;
		writeUniform.pImageInfo = nullptr;
		writeUniform.pTexelBufferView = nullptr;
		writeUniform.pNext = nullptr;

		descriptorWrite.push_back(writeUniform);

		if(d_meshes[i].texture.allset)
		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = d_meshes[i].texture.image.view;
			imageInfo.sampler = d_meshes[i].texture.sampler;

			VkWriteDescriptorSet writeSampler;
			writeSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSampler.dstSet = d_desctiptor_sets.sets[i];
			writeSampler.dstBinding = 1;
			writeSampler.dstArrayElement = 0;
			writeSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeSampler.descriptorCount = 1;
			writeSampler.pBufferInfo = nullptr;
			writeSampler.pImageInfo = &imageInfo;
			writeSampler.pTexelBufferView = nullptr;
			writeSampler.pNext = nullptr;

			descriptorWrite.push_back(writeSampler);
		}

		vkUpdateDescriptorSets(d_device, static_cast<uint32_t>(descriptorWrite.size()), descriptorWrite.data(), 0, nullptr);
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan descriptor sets updated");}
}

void Graph::createVertexBuffers()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_GRAPH;

    d_vertex_buffers.resize(d_meshes.size());
    for(size_t i = 0; i < d_meshes.size(); i++)
    {
        VkDeviceSize bufferSize = (uint64_t)(sizeof(Vertex)) * d_meshes[i].vertices.size();

        Buffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	    void* data;
	    vkMapMemory(d_device, stagingBuffer.mem, 0, bufferSize, 0, &data);
	    memcpy(data, d_meshes[i].vertices.data(), (size_t)bufferSize);
	    vkUnmapMemory(d_device, stagingBuffer.mem);

        Buffer vertexBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	    copyBufferToBuffer(stagingBuffer.buf, vertexBuffer.buf, bufferSize);
        stagingBuffer.destroy(d_device);

        d_vertex_buffers[i] = vertexBuffer;
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan graph uniform buffers created");}
}

void Graph::createIndiceBuffers()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_GRAPH;

    d_indice_buffers.resize(d_meshes.size());
    for(size_t i = 0; i < d_meshes.size(); i++)
    {
        VkDeviceSize bufferSize = (uint64_t)(sizeof(uint32_t)) * d_meshes[i].indices.size();

        Buffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	    void* data;
	    vkMapMemory(d_device, stagingBuffer.mem, 0, bufferSize, 0, &data);
	    memcpy(data, d_meshes[i].indices.data(), (size_t)bufferSize);
	    vkUnmapMemory(d_device, stagingBuffer.mem);

        Buffer indiceBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	    copyBufferToBuffer(stagingBuffer.buf, indiceBuffer.buf, bufferSize);
        stagingBuffer.destroy(d_device);

        d_indice_buffers[i] = indiceBuffer;
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan graph indice buffers created");}
}

void Graph::createRenderCommandBuffers()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_GRAPH;

    size_t swapChainImagesCount = app->GetRenderer()->getSwapChainImagesCount();
    d_commands = app->GetRenderer()->allocateRenderCommandBuffers(swapChainImagesCount * d_meshes.size());

    for(size_t j = 0; j < swapChainImagesCount; j++)
    {
    for (size_t i = 0; i < d_meshes.size(); i++)
	{
        size_t id = i + j * d_meshes.size();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(d_commands[id], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("ERROR: failed to begin recording Vulkan command buffer!");

		VkRenderPassBeginInfo renderPassInfo{};
		app->GetRenderer()->fillRenderPassInfo(renderPassInfo, j);

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {
			app->RENDER_CLEAR_VALUES[0], app->RENDER_CLEAR_VALUES[1],
			app->RENDER_CLEAR_VALUES[2], app->RENDER_CLEAR_VALUES[3],
		};
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(d_commands[id], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(d_commands[id], VK_PIPELINE_BIND_POINT_GRAPHICS, app->GetRenderer()->getGraphicsPipeline());

		VkBuffer vertexBuffers[] = { d_vertex_buffers[i].buf };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(d_commands[id], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(d_commands[id], d_indice_buffers[i].buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(d_commands[id], VK_PIPELINE_BIND_POINT_GRAPHICS, app->GetRenderer()->getGraphicsPipelineLayout(),
            0, 1, &d_desctiptor_sets.sets[i], 0, nullptr);

		vkCmdDrawIndexed(d_commands[id], static_cast<uint32_t>(d_meshes[i].indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(d_commands[id]);

		if (vkEndCommandBuffer(d_commands[id]) != VK_SUCCESS)
			throw std::runtime_error("ERROR: failed to record Vulkan render command buffer!");
	}
    }

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan render command buffers created");}
}

void Graph::createTextures(const std::set<std::string> paths)
{
	for(const auto& path : paths)
	{
    	Texture newTexture;

    	int texWidth, texHeight, texChannels;
    	stbi_uc* pixels = stbi_load((std::string(GLOB_FILE_FOLDER) + "/" + path).c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    	VkDeviceSize imageSize = (uint64_t)texWidth * (uint64_t)texHeight * 4;

    	if(!pixels)
    	{
    	    std::string message = "ERROR: failed to load image " + path + "!\nSTB failure reason: " + std::string(stbi_failure_reason());
    	    throw std::runtime_error(message);
    	}

    	Buffer stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    	    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    	void* data;
    	vkMapMemory(d_device, stagingBuffer.mem, 0, imageSize, 0, &data);
    	memcpy(data, pixels, static_cast<size_t>(imageSize));
    	vkUnmapMemory(d_device, stagingBuffer.mem);

    	stbi_image_free(pixels);

    	newTexture.image = createTextureImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
    	    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, stagingBuffer.buf);

    	VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(d_device, &samplerInfo, nullptr, &newTexture.sampler) != VK_SUCCESS)
			throw std::runtime_error("ERROR: failed to create Vulkan texture sampler!");

		newTexture.allset = true;

    	stagingBuffer.destroy(d_device);
    	
		d_unique_textures[path] = newTexture;
	}

}

Buffer Graph::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    Buffer newBuffer;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if(vkCreateBuffer(d_device, &bufferInfo, nullptr, &newBuffer.buf) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan buffer!");

    VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(d_device, newBuffer.buf, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = app->GetBackend()->findDeviceMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(d_device, &allocInfo, nullptr, &newBuffer.mem) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to allocate Vulkan memory for buffer!");
    
    vkBindBufferMemory(d_device, newBuffer.buf, newBuffer.mem, 0);
    newBuffer.allset = true;
    return newBuffer;
}

Image Graph::createTextureImage(uint32_t width, uint32_t height, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer stagingBuffer)
{
    Image newImage;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	if (vkCreateImage(d_device, &imageInfo, nullptr, &newImage.image) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(d_device, newImage.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = app->GetBackend()->findDeviceMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(d_device, &allocInfo, nullptr, &newImage.mem) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to allocate Vulkan image memory!");

	vkBindImageMemory(d_device, newImage.image, newImage.mem, 0);

    transitionTextureImageLayout(newImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, newImage.image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    transitionTextureImageLayout(newImage.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = newImage.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
	};

	if (vkCreateImageView(d_device, &viewInfo, nullptr, &newImage.view) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan image view!");

    newImage.allset = true;
    return newImage;
}

void Graph::transitionTextureImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = app->GetRenderer()->startSingleCommand();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::runtime_error("ERROR: unsupported Vulkan texture image layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	app->GetRenderer()->stopSingleCommand(commandBuffer);
}

void Graph::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = app->GetRenderer()->startSingleCommand();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	app->GetRenderer()->stopSingleCommand(commandBuffer);
}

void Graph::copyBufferToBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = app->GetRenderer()->startSingleCommand();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	app->GetRenderer()->stopSingleCommand(commandBuffer);
}

void Graph::onFrameSizeChangeStart()
{
	app->GetRenderer()->freeRenderCommandBuffers(d_commands);
	d_desctiptor_sets.destroy(d_device);
}

void Graph::onFrameSizeChangeEnd()
{
	createUniformBuffers();
    createDescriptorSets();
	createRenderCommandBuffers();
}
