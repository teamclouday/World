#include "base.hpp"

#include "global.hpp"
extern Application* app;

#include <set>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

VKAPI_ATTR VkBool32 VKAPI_CALL BASE::debug_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    std::string severity;
	std::string type;

    switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		severity = "verbose";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		severity = "info";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		severity = "warning";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		severity = "error";
		break;
	default:
		severity = "undefined";
		break;
	}

	switch (messageType)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		type = "general";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		type = "validation";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		type = "performance";
		break;
	default:
		type = "undefined";
		break;
	}

    if(myLogger)
    {
        std::string message = "Validation Layer [" + type + "][" + severity + "]: " + pCallbackData->pMessage;
        myLogger->AddMessage(myLoggerOwner, message, true);
    }

    return VK_FALSE;
}

VkResult BASE::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void BASE::DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

using namespace BASE;

Backend::Backend()
{
    createWindow();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createRenderPass();
}

Backend::~Backend()
{
    destroySwapChain();

    vkDestroyDevice(d_device, nullptr);

    if(app->BACKEND_ENABLE_VALIDATION)
        DestroyDebugUtilsMessengerEXT(d_instance, d_debug_messenger, nullptr);

    vkDestroySurfaceKHR(d_instance, d_surface, nullptr);
    vkDestroyInstance(d_instance, nullptr);

    glfwDestroyWindow(p_window);
    glfwTerminate();
}

void Backend::createWindow()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    if(glfwInit() != GLFW_TRUE)
        throw std::runtime_error("ERROR: failed to init GLFW!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "GLFW inited");}

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if(app->WINDOW_RESIZABLE)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    else
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    p_window = glfwCreateWindow(app->WINDOW_WIDTH, app->WINDOW_HEIGHT, app->WINDOW_TITLE.c_str(), nullptr, nullptr);
    if(!p_window)
        throw std::runtime_error("ERROR: failed to create GLFW window!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "GLFW window created");}

    glfwSetWindowUserPointer(p_window, (void*)this);
    glfwSetKeyCallback(p_window, glfw_key_callback);
}

void Backend::createInstance()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    requiredExtensions.insert(requiredExtensions.end(), d_device_extensions.begin(), d_device_extensions.end());
    checkInstanceExtensions(requiredExtensions);
    if(app->BACKEND_ENABLE_VALIDATION)
        checkInstanceLayers(d_validation_layers);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "World";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "World Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if(app->BACKEND_ENABLE_VALIDATION)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(d_validation_layers.size());
        createInfo.ppEnabledLayerNames = d_validation_layers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (void*)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if(vkCreateInstance(&createInfo, nullptr, &d_instance) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan instance!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan instance created");}

    setDebugMessenger();
}

void Backend::createSurface()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    if(glfwCreateWindowSurface(d_instance, p_window, nullptr, &d_surface) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan surface!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan surface created");}
}

void Backend::pickPhysicalDevice()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(d_instance, &deviceCount, nullptr);
    if(!deviceCount)
        throw std::runtime_error("ERROR: failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(d_instance, &deviceCount, devices.data());

    d_physical_device = VK_NULL_HANDLE;
    for(const auto& device : devices)
    {
        if(isPhysicalDeviceSuitable(device))
        {
            d_physical_device = device;
            break;
        }
    }

    if(d_physical_device == VK_NULL_HANDLE)
        throw std::runtime_error("ERROR: failed to find suitable GPU for Vulkan!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "GPU selected");}
}

void Backend::createLogicalDevice()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    VulkanQueueFamilyIndices indices = getQueueFamilies(d_physical_device);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilyIDs = {indices.graphicsFamilyID, indices.presentFamilyID};

    float queuePriority = 1.0f;
    for(uint32_t queueID : uniqueQueueFamilyIDs)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueID;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(d_device_extensions.size());
    createInfo.ppEnabledExtensionNames = d_device_extensions.data();

    if(app->BACKEND_ENABLE_VALIDATION)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(d_validation_layers.size());
        createInfo.ppEnabledLayerNames = d_validation_layers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if(vkCreateDevice(d_physical_device, &createInfo, nullptr, &d_device) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan logical device!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan logical device created");}
}

void Backend::createSwapChain()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    VulkanSwapChainSupport swapChainSupport = checkDeviceSwapChainSupport(d_physical_device);
    VkSurfaceFormatKHR surfaceFormat = selectSwapChainSurfaceFormat(swapChainSupport.surfaceFormats);
    VkPresentModeKHR presentMode = selectSwapChainPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = selectSwapChainExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = d_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VulkanQueueFamilyIndices indices = getQueueFamilies(d_physical_device);
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

    if(vkCreateSwapchainKHR(d_device, &createInfo, nullptr, &d_swap_chain) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan swap chain!");

    vkGetSwapchainImagesKHR(d_device,d_swap_chain, &imageCount, nullptr);
    d_swap_chain_images.resize(imageCount);
    vkGetSwapchainImagesKHR(d_device,d_swap_chain, &imageCount, d_swap_chain_images.data());
    d_swap_chain_image_format = surfaceFormat.format;
    d_swap_chain_image_extent = extent;
    d_swap_chain_image_views.resize(imageCount);
    for(size_t i = 0; i < imageCount; i++)
        d_swap_chain_image_views[i] = createImageView(d_swap_chain_images[i], d_swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan swap chain created");}
}

void Backend::createRenderPass()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

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
    depthAttachment.format = getDeviceSupportedImageFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
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

	if (vkCreateRenderPass(d_device, &renderPassInfo, nullptr, &d_render_pass) != VK_FALSE)
		throw std::runtime_error("ERROR: failed to create Vulkan render pass!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan render pass created");}
}

void Backend::checkInstanceExtensions(const std::vector<const char*> requiredExtensions)
{
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availbleExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availbleExtensions.data());

    for(const auto& extension : requiredExtensions)
    {
        bool found = false;
        for(const auto& availbleExtension : availbleExtensions)
        {
            if(!strcmp(extension, availbleExtension.extensionName))
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            std::string message = "ERROR: required Vulkan extension [" + std::string(extension) + "] not supported!";
            throw std::runtime_error(message);
        }
    }
}

void Backend::checkInstanceLayers(const std::vector<const char*> requiredLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const auto& layer : requiredLayers)
    {
        bool found = false;
        for(const auto& availableLayer : availableLayers)
        {
            if(!strcmp(layer, availableLayer.layerName))
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            std::string message = "ERROR: required Vulkan layer [" + std::string(layer) + "] not supported!";
            throw std::runtime_error(message);
        }
    }
}

void Backend::setDebugMessenger()
{
    if(!app->BACKEND_ENABLE_VALIDATION) return;

    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if(CreateDebugUtilsMessengerEXT(d_instance, &createInfo, nullptr, &d_debug_messenger) != VK_SUCCESS)
        throw std::runtime_error("ERROR: failed to create Vulkan debug messenger!");
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Debug messenger created");}
}

void Backend::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debug_messenger_callback;
}

bool Backend::isPhysicalDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    VulkanQueueFamilyIndices indices = getQueueFamilies(device);

    bool extensionSupported = checkDeviceExtensions(device);
    bool swapChainSupported = false;
    if(extensionSupported)
    {
        VulkanSwapChainSupport swapChainSupport = checkDeviceSwapChainSupport(device);
        swapChainSupported = !swapChainSupport.surfaceFormats.empty() && !swapChainSupport.presentModes.empty();
    }

    return deviceFeatures.geometryShader && deviceFeatures.samplerAnisotropy &&
        indices.graphicsInit && indices.presentInit && swapChainSupported;
}

VulkanQueueFamilyIndices Backend::getQueueFamilies(VkPhysicalDevice device)
{
    VulkanQueueFamilyIndices indices;
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);

    uint32_t i = 0;
    for(const auto& queueFamily : queueFamilies)
    {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamilyID = i;
            indices.graphicsInit = true;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, d_surface, &presentSupport);
        if(presentSupport == VK_TRUE)
        {
            indices.presentFamilyID = i;
            indices.presentInit = true;
        }

        if(indices.graphicsInit && indices.presentInit)
            break;

        i++;
    }
    return indices;
}

bool Backend::checkDeviceExtensions(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    std::vector<const char*> requiredExtensions(d_device_extensions.begin(), d_device_extensions.end());

    for(const auto& extension : requiredExtensions)
    {
        bool found = false;
        for(const auto& availableExtension : availableExtensions)
        {
            if(!strcmp(extension, availableExtension.extensionName))
            {
                found = true;
                break;
            }
        }
        if(!found) return false;
    }
    return true;
}

VulkanSwapChainSupport Backend::checkDeviceSwapChainSupport(VkPhysicalDevice device)
{
    VulkanSwapChainSupport supportDetails;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, d_surface, &supportDetails.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, d_surface, &formatCount, nullptr);
    if(formatCount)
    {
        supportDetails.surfaceFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, d_surface, &formatCount, supportDetails.surfaceFormats.data());
    }

    uint32_t modeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, d_surface, &modeCount, nullptr);
    if(modeCount)
    {
        supportDetails.presentModes.resize(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, d_surface, &modeCount, supportDetails.presentModes.data());
    }

    return supportDetails;
}

VkSurfaceFormatKHR Backend::selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats)
{
    for(const auto& availableFormat : availableFormats)
    {
        if(availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }
    return availableFormats[0];
}

VkPresentModeKHR Backend::selectSwapChainPresentMode(const std::vector<VkPresentModeKHR> availableModes)
{
    for(const auto& availableMode : availableModes)
    {
        if(availableMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availableMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Backend::selectSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != UINT32_MAX)
        return capabilities.currentExtent;
    else
    {
        int width, height;
        glfwGetFramebufferSize(p_window, &width, &height);
        VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

VkImageView Backend::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
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
    if(vkCreateImageView(d_device, &createInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("ERROR: failed to create Vulkan image view!");
    }
    return imageView;
}

VkFormat Backend::getDeviceSupportedImageFormat(const std::vector<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for(VkFormat format : candidates)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(d_physical_device, format, &properties);
        if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            return format;
        else if(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            return format;
    }
    throw std::runtime_error("ERROR: failed to find Vulkan supported image format!");
}

void Backend::destroySwapChain()
{

    vkDestroyRenderPass(d_device, d_render_pass, nullptr);

    for(size_t i = 0; i < d_swap_chain_image_views.size(); i++)
        vkDestroyImageView(d_device, d_swap_chain_image_views[i], nullptr);
    
    vkDestroySwapchainKHR(d_device, d_swap_chain, nullptr);
}
