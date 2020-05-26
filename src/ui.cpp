#include "ui.hpp"
#include "base.hpp"
#include "logging.hpp"

#include "global.hpp"
extern Application* app;

#include <stdexcept>
#include <iostream>

static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS) return;
    std::cerr << "[Imgui Vulkan] Error: VkResult = " << err << std::endl;
    if (err < 0)
        abort();
}

using namespace UTILS;

UI::UI()
{
    if(!app->GetRenderer()) throw std::runtime_error("ERROR: cannot create UI without renderer!");
    setupImGUI();
}

UI::~UI()
{
    BASE::Backend* p_backend = app->GetBackend();
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_UI;

    vkDestroyDescriptorPool(p_backend->d_device, d_imgui_descriptor_pool, nullptr);
    vkDestroyPipelineCache(p_backend->d_device, d_imgui_pipeline_cache, nullptr);
    vkDeviceWaitIdle(p_backend->d_device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "ImGui Vulkan context destroyed");}
}

ImDrawData* UI::drawMenu()
{
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("FPS");
        ImGui::Text("Current FPS: %.1f", app->RENDER_CURRENT_FPS);
        ImGui::End();
    }

    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    return drawData;
}

void UI::setupImGUI()
{
    BASE::Backend* p_backend = app->GetBackend();
    BASE::Renderer* p_renderer = app->GetRenderer();
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_UI;

    // first allocate a descriptor pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        VkResult err = vkCreateDescriptorPool(p_backend->d_device, &pool_info, nullptr, &d_imgui_descriptor_pool);
        check_vk_result(err);
    }
    // next create pipeline cache
    {
        VkPipelineCacheCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        vkCreatePipelineCache(p_backend->d_device, &createInfo, nullptr, &d_imgui_pipeline_cache);
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForVulkan(p_backend->p_window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = p_backend->d_instance;
    init_info.PhysicalDevice = p_backend->d_physical_device;
    init_info.Device = p_backend->d_device;
    init_info.QueueFamily = p_backend->getQueueFamilies(p_backend->d_physical_device).graphicsFamilyID;
    init_info.Queue = p_backend->d_graphics_queue;
    init_info.PipelineCache = d_imgui_pipeline_cache;
    init_info.DescriptorPool = d_imgui_descriptor_pool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = 2;
    init_info.ImageCount = p_renderer->d_swap_chain_images.size();
    init_info.CheckVkResultFn = check_vk_result;
    init_info.MSAASamples = app->RENDER_ENABLE_MSAA ? p_renderer->d_msaa_sample_count : VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info, p_renderer->d_render_pass);
    {
        VkCommandBuffer commandBuffer = p_renderer->startSingleCommand();
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        p_renderer->stopSingleCommand(commandBuffer);
    }

    if(myLogger){myLogger->AddMessage(myLoggerOwner, "ImGui Vulkan context created");}
}

ImDrawData* UI::recordUI()
{
    ImDrawData* data = drawMenu();
    return data;
}
