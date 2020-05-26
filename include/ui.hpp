// File Description
// contains functions to manage the UI

#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace UTILS
{
    class UI
    {
    public:
        UI();
        ~UI();

        ImDrawData* recordUI();

    private:
        void setupImGUI();
        ImDrawData* drawMenu();

    private:
        VkDescriptorPool d_imgui_descriptor_pool;
        VkPipelineCache d_imgui_pipeline_cache;
    };
}