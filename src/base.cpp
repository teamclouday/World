#include "base.hpp"

#include "global.hpp"
extern Application* app;

#include <stdexcept>

using namespace BASE;

Backend::Backend()
{
    createWindow();
}

Backend::~Backend()
{
    glfwDestroyWindow(p_window);
    glfwTerminate();
}

void Backend::createWindow()
{
    LOGGING::Logger* myLogger = app->GetLogger();
    LOGGING::LogOwners myLoggerOwner = LOGGING::LOG_OWNERS_BACKEND;

    if(glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("failed to init GLFW!");
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
        throw std::runtime_error("failed to create GLFW window!");
    }
    if(myLogger){myLogger->AddMessage(myLoggerOwner, "GLFW window created");}
    glfwSetWindowUserPointer(p_window, (void*)this);
    glfwSetKeyCallback(p_window, glfw_key_callback);
}