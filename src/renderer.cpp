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
    createSwapChain();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
}

void Renderer::refresh()
{
    destroySwapChain();

    


}

void Renderer::drawFrame()
{

}

Renderer::~Renderer()
{
    destroySwapChain();

    vkDestroyDescriptorSetLayout(p_backend->d_device, d_descriptor_set_layout, nullptr);

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

void Renderer::createDescriptorSetLayout()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(p_backend->d_device, &layoutInfo, nullptr, &d_descriptor_set_layout) != VK_SUCCESS)
		throw std::runtime_error("ERROR: failed to create vulkan descriptor set layout!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan descriptor set layout created");}
}

void Renderer::createGraphicsPipeline()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    UTILS::ShaderSourceDetails shaderSourceDetails = app->SHADER_SOURCE_DETAILS;
    if(!shaderSourceDetails.validate())
        throw std::runtime_error("ERROR: shader source details are not set properly!");
    std::string path = app->SHADER_SOURCE_PATH + "/";

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;
    shaderStages.resize(0);

    for(size_t i = 0; i < shaderSourceDetails.names.size(); i++)
    {
        auto shaderCode = FILES::read_bytes_from_file(path + shaderSourceDetails.names[0]);
        VkShaderModule shaderModule = createShaderModule(shaderCode, shaderSourceDetails.names[0]);
        shaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        switch(shaderSourceDetails.types[i])
        {
            case UTILS::SHADER_VERTEX:
                stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case UTILS::SHADER_TESS_CONTROL:
                stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                break;
            case UTILS::SHADER_TESS_EVALUATE:
                stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                break;
            case UTILS::SHADER_GEOMETRY:
                stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            case UTILS::SHADER_FRAGMENT:
                stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case UTILS::SHADER_COMPUTE:
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

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)d_swap_chain_image_extent.width;
	viewport.height = (float)d_swap_chain_image_extent.height;
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
	pipelineLayoutInfo.pSetLayouts = &d_descriptor_set_layout;
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

void Renderer::destroySwapChain()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_RENDERER;

    vkDestroyPipeline(p_backend->d_device, d_pipeline, nullptr);
    vkDestroyPipelineLayout(p_backend->d_device, d_pipeline_layout, nullptr);
    vkDestroyRenderPass(p_backend->d_device, d_render_pass, nullptr);

    for(size_t i = 0; i < d_swap_chain_image_views.size(); i++)
        vkDestroyImageView(p_backend->d_device, d_swap_chain_image_views[i], nullptr);
    
    vkDestroySwapchainKHR(p_backend->d_device, d_swap_chain, nullptr);

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan swap chain destroyed");}
}
