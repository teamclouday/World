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
#include <string>
#include <array>

#include "data.hpp"

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

    class Renderer;

    class Backend
    {
    friend class Renderer;
    friend class DATA::Graph;
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
        // find physical device memory type
        uint32_t findDeviceMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

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
        VkQueue d_graphics_queue;
        VkQueue d_present_queue;
    };

    class Renderer
    {
    public:
        Renderer();
        ~Renderer();

        void drawFrame();
        void refresh();

        // allocate render command buffers
        void allocateRenderCommandBuffers(std::vector<VkCommandBuffer>& buffers);
        // free render command buffers
        void freeRenderCommandBuffers(std::vector<VkCommandBuffer>& buffers);
        // begin a single immediate command
        VkCommandBuffer startSingleCommand();
        // stop a single immediate command
        void stopSingleCommand(VkCommandBuffer& commandBuffer);
        // get information for graph
        void fillRenderPassInfo(VkRenderPassBeginInfo& info, size_t id)
        {
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		    info.renderPass = d_render_pass;
		    info.framebuffer = d_swap_chain_framebuffers[id];
		    info.renderArea.offset = { 0, 0 };
		    info.renderArea.extent = d_swap_chain_image_extent;
        }
        // get pipeline
        VkPipeline getGraphicsPipeline(){return d_pipeline;}
        // get pipeline layout
        VkPipelineLayout getGraphicsPipelineLayout(){return d_pipeline_layout;}
        // get swap chain images count
        size_t getSwapChainImagesCount(){return d_swap_chain_images.size();}

    private:
        // create swap chain
        void createSwapChain();
        // create render pass
        void createRenderPass();
        // create pipeline
        void createGraphicsPipeline();
        // create command pool
        void createCommandPool();
        // create command buffers
        void createCommandBuffers();
        // create depth resources
        void createDepthResources();
        // create framebuffers
        void createFramebuffers();
        // create synchronization objects
        void createSyncObjects();
        
        // select swap chain surface format from options
        VkSurfaceFormatKHR selectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats);
        // select swap chain present mode from options
        VkPresentModeKHR selectSwapChainPresentMode(const std::vector<VkPresentModeKHR> availableModes);
        // select swap chain extent
        VkExtent2D selectSwapChainExtent(VkSurfaceCapabilitiesKHR& capabilities);
        // create image
        void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        // create image view from image
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        // create shader module from source
        VkShaderModule createShaderModule(const std::vector<char> code, const std::string name);
        // transition image layout
        void transitionImageLayout(VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
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
        std::vector<VkFramebuffer> d_swap_chain_framebuffers;

        VkRenderPass d_render_pass;

        VkPipeline d_pipeline;
        VkPipelineLayout d_pipeline_layout;

        VkCommandPool d_command_pool;
        std::vector<VkCommandBuffer> d_render_commands;

        const size_t MAX_FRAMES_IN_FLIGHT = 2;
        std::vector<VkSemaphore> d_semaphore_image;
        std::vector<VkSemaphore> d_semaphore_render;
        std::vector<VkFence> d_fence_render;
        std::vector<VkFence> d_fence_image;

        DATA::Image d_depth_image;

        DATA::Graph* p_graph;
    };

    // a GLFW key callback function
    // TODO: add support for user-defined key-bindings
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