// File Description
// basic backend functions
// 1. window management
// 2. vulkan instance management
// 3. handle low level vulkan function calls
// basic renderer functions
// 1. manage long term variables (shader module)
// 2. manage render term variables (pipeline), could be updated by recreating swap chain
// 3. manage fast changing variables (uniform buffer) also long term var

#pragma once

#include <Vulkan/Vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>
#include <array>

namespace BASE
{
    struct VulkanQueueFamilyIndices
    {
        uint32_t graphicsFamilyID;
        bool graphicsInit = false;
        uint32_t presentFamilyID;
        bool presentInit = false;
    };

    struct VulkanSwapChainSupport
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct VulkanVertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(VulkanVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
        {
            
        }
    };

    class Renderer;

    class Backend
    {
    friend class Renderer;
    public:
        Backend();
        ~Backend();

    private:
        // create GLFW window and GLFW context
        void createWindow();
        // create Vulkan basic instance
        void createInstance();
        // create Vulkan surface
        void createSurface();
        // pick valid physical device
        void pickPhysicalDevice();
        // create logical device
        void createLogicalDevice();

        // check instance extensions
        void checkInstanceExtensions(const std::vector<const char*> requiredExtensions);
        // check instance layers
        void checkInstanceLayers(const std::vector<const char*> requiredLayers);
        // fill in debug messenger struct object
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        // set debug messenger
        void setDebugMessenger();
        // check if physical device is suitable
        bool isPhysicalDeviceSuitable(VkPhysicalDevice device);
        // get queue family for physical device
        VulkanQueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
        // check device extensions
        bool checkDeviceExtensions(VkPhysicalDevice device);
        // check swap chain support for physical device
        VulkanSwapChainSupport checkDeviceSwapChainSupport(VkPhysicalDevice device);
        // get physical device supported image format
        VkFormat getDeviceSupportedImageFormat(const std::vector<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    public:
        const std::vector<const char*> d_validation_layers = {
            "VK_LAYER_KHRONOS_validation" // khronos validation layer
        };
        const std::vector<const char*> d_device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME // swapchain extension
        };

        bool d_frame_refreshed = false;

    private:
        // GLFW related variables
        GLFWwindow* p_window;

        // Vulkan variables
        VkInstance d_instance;
        VkSurfaceKHR d_surface;
        VkDevice d_device;
        VkPhysicalDevice d_physical_device;
        VkDebugUtilsMessengerEXT d_debug_messenger;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void drawFrame();
        void refresh();

    private:
        // create swap chain
        void createSwapChain();
        // create render pass
        void createRenderPass();
        // create pipeline
        void createGraphicsPipeline();

        
        // select swap chain surface format from options
        VkSurfaceFormatKHR selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats);
        // select swap chain present mode from options
        VkPresentModeKHR selectSwapChainPresentMode(const std::vector<VkPresentModeKHR> availableModes);
        // select swap chain extent
        VkExtent2D selectSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities);
        // create image view from image
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        // create shader module from source
        VkShaderModule createShaderModule(const std::vector<char> code, const std::string name);
        // destroy swap chain
        void destroySwapChain();

    public:

    private:
        Backend* p_backend;

        VkSwapchainKHR d_swap_chain;
        std::vector<VkImage> d_swap_chain_images;
        std::vector<VkImageView> d_swap_chain_image_views;
        VkFormat d_swap_chain_image_format;
        VkExtent2D d_swap_chain_image_extent;
        VkRenderPass d_render_pass;

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

    // GLFW frame buffer resize callback
    static void glfw_frame_resize_callback(GLFWwindow* window, int width, int height)
    {
        auto backend = reinterpret_cast<Backend*>(glfwGetWindowUserPointer(window));
        backend->d_frame_refreshed = true;
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