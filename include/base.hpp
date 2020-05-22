// File Description
// basic backend functions
// 1. window management
// 2. vulkan instance management
// 3. handle low level vulkan function calls

#pragma once

#include <Vulkan/Vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace BASE
{
    class Backend
    {
    public:
        Backend();
        ~Backend();

    private:
        // create GLFW window and GLFW context
        void createWindow();
        // create Vulkan basic instance
        void createInstance();




        // check instance extensions
        void checkInstanceExtensions(std::vector<const char*> requiredExtensions);
        // check instance layers
        void checkInstanceLayers(std::vector<const char*> requiredLayers);
        // fill in debug messenger struct object
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        // set debug messenger
        void setDebugMessenger();

    public:
        const std::vector<const char*> d_validation_layers = {
            "VK_LAYER_KHRONOS_validation" // khronos validation layer
        };
        const std::vector<const char*> d_device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME // swapchain extension
        };

    private:
        // GLFW related variables
        GLFWwindow* p_window;

        // Vulkan variables
        VkInstance d_instance;
        VkDebugUtilsMessengerEXT d_debug_messenger;
    };

    // a GLFW key callback function
    // TODO: May add support for user-defined key-bindings
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    // Vulkan debug messenger callback function
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageType,
	    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	    void* pUserData
    );
    // create debug messenger helper function
    VkResult CreateDebugUtilsMessengerEXT(
	    VkInstance instance,
	    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	    const VkAllocationCallbacks* pAllocator,
	    VkDebugUtilsMessengerEXT* pDebugMessenger
    );
    // destroy debug messenger helper function
    void DestroyDebugUtilsMessengerEXT(
	    VkInstance instance,
	    VkDebugUtilsMessengerEXT debugMessenger,
	    const VkAllocationCallbacks* pAllocator
    );

}