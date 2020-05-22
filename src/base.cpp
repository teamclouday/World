#include "base.hpp"

#include "global.hpp"
extern Application* app;

#include <stdexcept>

using namespace BASE;

Backend::Backend()
{
    createWindow();
    createInstance();
}

Backend::~Backend()
{
    vkDestroyInstance(d_instance, nullptr);

    glfwDestroyWindow(p_window);
    glfwTerminate();
}

void Backend::createWindow()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    if(glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("ERROR: failed to init GLFW!");
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "GLFW inited");}
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if(app->WINDOW_RESIZABLE)
    {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }
    p_window = glfwCreateWindow(app->WINDOW_WIDTH, app->WINDOW_HEIGHT, app->WINDOW_TITLE.c_str(), nullptr, nullptr);
    if(!p_window)
    {
        throw std::runtime_error("ERROR: failed to create GLFW window!");
    }
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
    {
        throw std::runtime_error("ERROR: failed to create Vulkan instance!");
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Vulkan instance created");}

    setDebugMessenger();
}

void Backend::checkInstanceExtensions(std::vector<const char*> requiredExtensions)
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
            std::string message = "ERROR: required extension [" + std::string(extension) + "] not supported!";
            throw std::runtime_error(message);
        }
    }
}

void Backend::checkInstanceLayers(std::vector<const char*> requiredLayers)
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
            std::string message = "ERROR: required layer [" + std::string(layer) + "] not supported!";
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
    {
        throw std::runtime_error("ERROR: failed to create debug messenger!");
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "Debug messenger created");}
}

void Backend::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debug_messenger_callback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
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
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
