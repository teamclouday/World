// File Description
// basic backend functions
// 1. window management
// 2. vulkan instance management
// 3. handle low level vulkan function calls

#pragma once

#include <Vulkan/Vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

    private:
        GLFWwindow* p_window;

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
}