#include "base.hpp"

#include "global.hpp"
extern Application* app;

#include <set>
#include <stdexcept>

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

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    UTILS::Camera* myCamera = app->GetCamera();
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_ESCAPE)
        {
            if(!myCamera || !myCamera->focus)
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            else
            {
                myCamera->focus = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            
        }
        if(!myCamera) return;
        if(key == GLFW_KEY_W || key == GLFW_KEY_UP)
            if(myCamera->focus) myCamera->keyMap[0] = true;
        if(key == GLFW_KEY_A || key == GLFW_KEY_LEFT)
            if(myCamera->focus) myCamera->keyMap[1] = true;
        if(key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
            if(myCamera->focus) myCamera->keyMap[2] = true;
        if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)
            if(myCamera->focus) myCamera->keyMap[3] = true;
    }
    if(action == GLFW_RELEASE)
    {
        if(!myCamera) return;
        if(key == GLFW_KEY_W || key == GLFW_KEY_UP)
            myCamera->keyMap[0] = false;
        if(key == GLFW_KEY_A || key == GLFW_KEY_LEFT)
            myCamera->keyMap[1] = false;
        if(key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
            myCamera->keyMap[2] = false;
        if(key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)
            myCamera->keyMap[3] = false;
    }
    if(myCamera) myCamera->update(app->CAMERA_SPEED, 0.0f, 0.0f);
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    UTILS::Camera* myCamera = app->GetCamera();
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if(!myCamera) return;
        if(!myCamera->focus)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            myCamera->focus = true;
        }
    }
}

static void glfw_mouse_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    UTILS::Camera* myCamera = app->GetCamera();
    if(!myCamera) return;
    if(myCamera->mousePosUpdated)
    {
        float xoffset = xpos - myCamera->mousePos[0];
        float yoffset = ypos - myCamera->mousePos[1];
        myCamera->update(app->CAMERA_SPEED, xoffset, yoffset);
    }
    myCamera->mousePos[0] = xpos;
    myCamera->mousePos[1] = ypos;
    if(!myCamera->mousePosUpdated) myCamera->mousePosUpdated = true;
}

using namespace BASE;

Backend::Backend()
{
    createWindow();
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

Backend::~Backend()
{
    Renderer* myRenderer = app->GetRenderer();
    if(myRenderer)
        delete myRenderer;

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
    glfwSetFramebufferSizeCallback(p_window, glfw_frame_resize_callback);
    glfwSetKeyCallback(p_window, glfw_key_callback);
    glfwSetMouseButtonCallback(p_window, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(p_window, glfw_mouse_pos_callback);
}

void Backend::createInstance()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if(app->BACKEND_ENABLE_VALIDATION)
    {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        checkInstanceLayers(d_validation_layers);
    }
    checkInstanceExtensions(requiredExtensions);

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

    vkGetDeviceQueue(d_device, indices.graphicsFamilyID, 0, &d_graphics_queue);
    vkGetDeviceQueue(d_device, indices.presentFamilyID, 0, &d_present_queue);
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
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

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

uint32_t Backend::findDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(d_physical_device, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) &&
			((memProperties.memoryTypes[i].propertyFlags & properties) == properties))
		{
			return i;
		}
	}

	throw std::runtime_error("ERROR: failed to find suitable Vulkan device memory type!");
}
