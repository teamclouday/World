#include "global.hpp"

Application* app = new Application();

#include <iostream>

int main()
{
    int ret = APP_EXIT_SUCCESS;

    app->WINDOW_RESIZABLE = true;
    app->LOGGER_SAVE_LOG = true;

    UTILS::ShaderSourceDetails details;
    details.names.push_back("simple.vert.spv");
    details.types.push_back(UTILS::SHADER_VERTEX);
    details.names.push_back("simple.frag.spv");
    details.types.push_back(UTILS::SHADER_FRAGMENT);

    app->SHADER_SOURCE_DETAILS = details;
    app->SHADER_SOURCE_PATH = "shaders/simple";

    try
    {
        app->StartBackend();
        app->StartRenderer();
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        ret = APP_EXIT_FAILURE;
    }

    delete app;
    
    return ret;
}