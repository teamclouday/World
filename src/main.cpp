#include "global.hpp"

Application* app = new Application();

int main()
{
    app->WINDOW_RESIZABLE = true;
    app->LOGGER_SAVE_LOG = true;
    app->StartBackend();
    delete app;
    return 0;
}