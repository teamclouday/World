#include "global.hpp"

Application* app = new Application();

#include <iostream>

int main()
{
    int ret = APP_EXIT_SUCCESS;

    app->WINDOW_RESIZABLE = true;
    app->LOGGER_SAVE_LOG = true;

    DATA::ShaderSourceDetails details;
    details.names.push_back("simple.vert.spv");
    details.types.push_back(DATA::SHADER_VERTEX);
    details.names.push_back("simple.frag.spv");
    details.types.push_back(DATA::SHADER_FRAGMENT);
    details.path = "shaders/simple";

    app->GRAPH_SHADER_DETAILS = details;

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