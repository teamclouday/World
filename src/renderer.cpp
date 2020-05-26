#include "base.hpp"

#include "global.hpp"
extern Application* app;

#include "files.hpp"

#include <stdexcept>
#include <algorithm>
#include <array>
#include <cstdint>

using namespace BASE;

Renderer::Renderer()
{
    p_backend = app->GetBackend();
    if(!p_backend)
        throw std::runtime_error("ERROR: backend not initialized when using renderer!");
    createCommandPool();
    createSwapChain();
    createRenderPass();
    createDepthResources();
    createSyncObjects();
}

void Renderer::CreateGraph()
{
    if(app->GRAPH_MESHES.size())
        p_graph = DATA::Graph::newGraph(app->GRAPH_MESHES, p_backend->d_device);
    else if(app->GRAPH_MODEL_PATH != "")
        p_graph = DATA::Graph::newGraph(app->GRAPH_MODEL_PATH, p_backend->d_device);
    else
        throw std::runtime_error("ERROR: no graph information is set for renderer!");
    createGraphicsPipeline();
    createFramebuffers();
}

void Renderer::loop(USER_UPDATE user_func)
{
    if(!p_graph) return;

    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;
    UTILS::Camera* myCamera = app->GetCamera();
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "loop started");}

    p_graph->createRenderCommandBuffers();
    while(!glfwWindowShouldClose(p_backend->p_window))
    {
        glfwPollEvents();
        if(myCamera) myCamera->update(app->CAMERA_SPEED, 0.0f, 0.0f);
        drawFrame(user_func);
    }

    vkDeviceWaitIdle(p_backend->d_device);

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "loop ended");}
}

void Renderer::drawFrame(USER_UPDATE user_func)
{
    vkWaitForFences(p_backend->d_device, 1, &d_fence_render[CURRENT_FRAME], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(p_backend->d_device, d_swap_chain, UINT64_MAX, d_semaphore_image[CURRENT_FRAME], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		throw std::runtime_error("ERROR: failed to acquire Vulkan swap chain image!");

	if (d_fence_image[imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(p_backend->d_device, 1, &d_fence_image[imageIndex], VK_TRUE, UINT64_MAX);
	d_fence_image[imageIndex] = d_fence_render[CURRENT_FRAME];

	updateUniformBuffers(user_func);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { d_semaphore_image[CURRENT_FRAME] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &p_graph->d_commands[imageIndex];

	VkSemaphore signalSemaphores[] = { d_semaphore_render[CURRENT_FRAME] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(p_backend->d_device, 1, &d_fence_render[CURRENT_FRAME]);

	if (vkQueueSubmit(p_backend->d_graphics_queue, 1, &submitInfo, d_fence_render[CURRENT_FRAME]) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to submit Vulkan draw command buffer!");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { d_swap_chain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(p_backend->d_present_queue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || p_backend->d_frame_refreshed)
	{
		p_backend->d_frame_refreshed = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to present Vulkan swap chain image!");

	vkQueueWaitIdle(p_backend->d_present_queue);

	CURRENT_FRAME = (CURRENT_FRAME + 1) % MAX_FRAMES_IN_FLIGHT;
}

Renderer::~Renderer()
{
    if(p_graph)
        delete p_graph;
    p_graph = nullptr;

    destroySwapChain();

    p_backend = nullptr;
}

void Renderer::createSwapChain()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    VulkanSwapChainSupport swapChainSupport = p_backend->checkDeviceSwapChainSupport(p_backend->d_physical_device);
    VkSurfaceFormatKHR surfaceFormat = selectSwapChainSurfaceFormat(swapChainSupport.surfaceFormats);
    VkPresentModeKHR presentMode = selectSwapChainPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = selectSwapChainExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = p_backend->d_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VulkanQueueFamilyIndices indices = p_backend->getQueueFamilies(p_backend->d_physical_device);
    uint32_t queueFamilyIDs[] = {indices.graphicsFamilyID, indices.presentFamilyID};
    if(indices.graphicsFamilyID != indices.presentFamilyID)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIDs;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(p_backend->d_device, &createInfo, nullptr, &d_swap_chain) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan swap chain!");

    vkGetSwapchainImagesKHR(p_backend->d_device, d_swap_chain, &imageCount, nullptr);
    d_swap_chain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(p_backend->d_device, d_swap_chain, &imageCount, d_swap_chain_images.data());
    d_swap_chain_image_format = surfaceFormat.format;
    d_swap_chain_image_extent = extent;
    d_swap_chain_image_views.resize(imageCount);
    for(size_t i = 0; i < imageCount; i++)
        d_swap_chain_image_views[i] = createImageView(d_swap_chain_images[i], d_swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan swap chain created");}
}

void Renderer::createRenderPass()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = d_swap_chain_image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = p_backend->getDeviceSupportedImageFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(p_backend->d_device, &renderPassInfo, nullptr, &d_render_pass) != VK_FALSE)
		throw std::runtime_error("ERROR: failed to create Vulkan render pass!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan render pass created");}
}

void Renderer::createGraphicsPipeline()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    DATA::ShaderSourceDetails shaderSourceDetails = app->GRAPH_SHADER_DETAILS;
    if(!shaderSourceDetails.validate())
        throw std::runtime_error("ERROR: shader source details are not set properly!");
    std::string path = app->GRAPH_SHADER_DETAILS.path + "/";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;
    shaderStages.resize(0);

    for(size_t i = 0; i < shaderSourceDetails.names.size(); i++)
    {
        auto shaderCode = FILES::read_bytes_from_file(path + shaderSourceDetails.names[i]);
        VkShaderModule shaderModule = createShaderModule(shaderCode, shaderSourceDetails.names[i]);
        shaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        switch(shaderSourceDetails.types[i])
        {
            case DATA::SHADER_VERTEX:
                stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case DATA::SHADER_TESS_CONTROL:
                stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case DATA::SHADER_TESS_EVALUATE:
                stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case DATA::SHADER_GEOMETRY:
                stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case DATA::SHADER_FRAGMENT:
                stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case DATA::SHADER_COMPUTE:
                stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                stageInfo.stage = VK_SHADER_STAGE_ALL;
                break;
        }
        stageInfo.module = shaderModule;
        stageInfo.pName = "main";
        shaderStages.push_back(stageInfo);
        if(myLogger){myLogger->AddMessage(myLoggerOwner, "shader file " + shaderSourceDetails.names[i] + " loaded");}
    }

    auto bindingDescription = DATA::Vertex::getBindingDescription();
    auto attributeDescriptions = DATA::Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = (float)d_swap_chain_image_extent.height;
	viewport.width = (float)d_swap_chain_image_extent.width;
	viewport.height = -(float)d_swap_chain_image_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = d_swap_chain_image_extent;

    VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &p_graph->d_descriptor_layout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(p_backend->d_device, &pipelineLayoutInfo, nullptr, &d_pipeline_layout) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan pipeline layout!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan pipeline layout created");}

    VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = d_pipeline_layout;
	pipelineInfo.renderPass = d_render_pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(p_backend->d_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &d_pipeline) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan graphics pipeline!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan pipeline created");}

    for(size_t i = 0; i < shaderModules.size(); i++)
        vkDestroyShaderModule(p_backend->d_device, shaderModules[i], nullptr);
}

void Renderer::createCommandPool()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    VulkanQueueFamilyIndices queueFamilyIndices = p_backend->getQueueFamilies(p_backend->d_physical_device);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyID;
	poolInfo.flags = 0;

	if (vkCreateCommandPool(p_backend->d_device, &poolInfo, nullptr, &d_command_pool) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan command pool!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan command pool created");}
}

void Renderer::createDepthResources()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    VkFormat depthFormat = p_backend->getDeviceSupportedImageFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    createImage(d_swap_chain_image_extent.width, d_swap_chain_image_extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        d_depth_image.image, d_depth_image.mem);
    d_depth_image.view = createImageView(d_depth_image.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    transitionImageLayout(d_depth_image.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    d_depth_image.allset = true;
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan depth resources created");}
}

void Renderer::createFramebuffers()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    d_swap_chain_framebuffers.resize(d_swap_chain_images.size());
    for(size_t i = 0; i < d_swap_chain_images.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
            d_swap_chain_image_views[i],
            d_depth_image.view
        };
        VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = d_render_pass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = d_swap_chain_image_extent.width;
		framebufferInfo.height = d_swap_chain_image_extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(p_backend->d_device, &framebufferInfo, nullptr, &d_swap_chain_framebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("ERROR: failed to create Vulkan framebuffer!");
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan framebuffers created");}
}

void Renderer::createSyncObjects()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    d_semaphore_image.resize(MAX_FRAMES_IN_FLIGHT);
	d_semaphore_render.resize(MAX_FRAMES_IN_FLIGHT);
	d_fence_render.resize(MAX_FRAMES_IN_FLIGHT);
	d_fence_image.resize(d_swap_chain_images.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(p_backend->d_device, &semaphoreInfo, nullptr, &d_semaphore_image[i]) != VK_SUCCESS ||
			vkCreateSemaphore(p_backend->d_device, &semaphoreInfo, nullptr, &d_semaphore_render[i]) != VK_SUCCESS ||
			vkCreateFence(p_backend->d_device, &fenceInfo, nullptr, &d_fence_render[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("ERROR: failed to create Vulkan synchronization objects for each frame!");
		}
	}
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan sync objects created");}
}

VkSurfaceFormatKHR Renderer::selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats)
{
    for(const auto& availableFormat : availableFormats)
    {
        if(availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }
    return availableFormats[0];
}

VkPresentModeKHR Renderer::selectSwapChainPresentMode(const std::vector<VkPresentModeKHR> availableModes)
{
    for(const auto& availableMode : availableModes)
    {
        if(availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availableMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Renderer::selectSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;
    else
    {
        int width, height;
        glfwGetFramebufferSize(p_backend->p_window, &width, &height);
        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void Renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	if (vkCreateImage(p_backend->d_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create Vulkan image!");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(p_backend->d_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = p_backend->findDeviceMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(p_backend->d_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(p_backend->d_device, image, imageMemory, 0);
}

VkImageView Renderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    createInfo.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
	};

    VkImageView imageView;
    if(vkCreateImageView(p_backend->d_device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create Vulkan image view!");
    }
    return imageView;
}

VkShaderModule Renderer::createShaderModule(const std::vector<char> code, const std::string name)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(p_backend->d_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        std::string message = "ERROR: failed to create shader module for " + name;
        throw std::runtime_error(message);
    }
    return shaderModule;
}

void Renderer::transitionImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = startSingleCommand();

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

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	
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
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		throw std::runtime_error("ERROR: unsupported Vulkan layout transition!");

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	stopSingleCommand(commandBuffer);
}

VkCommandBuffer Renderer::startSingleCommand()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = d_command_pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(p_backend->d_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Renderer::stopSingleCommand(VkCommandBuffer& commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(p_backend->d_graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(p_backend->d_graphics_queue);

    vkFreeCommandBuffers(p_backend->d_device, d_command_pool, 1, &commandBuffer);
}

std::vector<VkCommandBuffer> Renderer::allocateRenderCommandBuffers(size_t size)
{
    std::vector<VkCommandBuffer> buffers;
    buffers.resize(size);

    VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = d_command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(size);

	if (vkAllocateCommandBuffers(p_backend->d_device, &allocInfo, buffers.data()) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to allocate Vulkan command buffers!");
    return buffers;
}

void Renderer::freeRenderCommandBuffers(std::vector<VkCommandBuffer> buffers)
{
    vkFreeCommandBuffers(p_backend->d_device, d_command_pool, static_cast<uint32_t>(buffers.size()), buffers.data());
}

void Renderer::destroySwapChain()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    d_depth_image.destroy(p_backend->d_device);

    for(size_t i = 0; i < d_swap_chain_framebuffers.size(); i++)
        vkDestroyFramebuffer(p_backend->d_device, d_swap_chain_framebuffers[i], nullptr);

    vkDestroyPipeline(p_backend->d_device, d_pipeline, nullptr);
    vkDestroyPipelineLayout(p_backend->d_device, d_pipeline_layout, nullptr);
    vkDestroyRenderPass(p_backend->d_device, d_render_pass, nullptr);

    for(size_t i = 0; i < d_swap_chain_image_views.size(); i++)
        vkDestroyImageView(p_backend->d_device, d_swap_chain_image_views[i], nullptr);
    
    vkDestroySwapchainKHR(p_backend->d_device, d_swap_chain, nullptr);

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan swap chain destroyed");}
}

void Renderer::recreateSwapChain()
{
    int width = 0;
	int height = 0;
	glfwGetFramebufferSize(p_backend->p_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(p_backend->p_window, &width, &height);
		glfwWaitEvents();
	}

    vkDeviceWaitIdle(p_backend->d_device);

    p_graph->onFrameSizeChangeStart();
	destroySwapChain();

	createSwapChain();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();

    p_graph->onFrameSizeChangeEnd();
}

void Renderer::updateUniformBuffers(USER_UPDATE user_func)
{
    for(size_t i = 0; i < d_swap_chain_images.size(); i++)
    {
        user_func(p_graph->d_ubo_data[i], d_swap_chain_image_extent.width, d_swap_chain_image_extent.height);

        void* data;
        vkMapMemory(p_backend->d_device, p_graph->d_ubo_buffers[i].mem, 0, sizeof(DATA::CameraUniform), 0, &data);
        memcpy(data, &p_graph->d_ubo_data[i], sizeof(DATA::CameraUniform));
        vkUnmapMemory(p_backend->d_device, p_graph->d_ubo_buffers[i].mem);
    }
}
