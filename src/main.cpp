#include "global.hpp"

Application* app = new Application();

#include <iostream>

int main()
{
    int ret = APP_EXIT_SUCCESS;
    app->WINDOW_RESIZABLE = true;
    app->LOGGER_SAVE_LOG = true;
    try
    {
        app->StartBackend();

    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        ret = APP_EXIT_FAILURE;
    }
    delete app;
    return ret;
}