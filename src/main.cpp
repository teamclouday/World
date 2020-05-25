#include "global.hpp"

Application* app = new Application();

#include <iostream>

// user defined camera uniform update function
void update_uniform(DATA::CameraUniform& data)
{
    BASE::Renderer* p_renderer = app->GetRenderer();
    uint32_t width, height;
    p_renderer->getSwapChainImageExtent(width, height);
    data.view = app->GetCamera()->GetViewMatrix();
    data.proj = glm::perspective(glm::radians(60.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
}

int main()
{
    int ret = APP_EXIT_SUCCESS;

    // set basic variables
    app->WINDOW_TITLE = "Simple Vulkan Test";
    app->WINDOW_RESIZABLE = true;
    app->LOGGER_SAVE_LOG = true;

    // set shader resources
    DATA::ShaderSourceDetails details;
    details.names.push_back("simple.vert.spv");
    details.types.push_back(DATA::SHADER_VERTEX);
    details.names.push_back("simple.frag.spv");
    details.types.push_back(DATA::SHADER_FRAGMENT);
    details.path = "shaders/simple";
    app->GRAPH_SHADER_DETAILS = details;

#if 0
    std::vector<DATA::Vertex> vertices = {
        // position           normal tangent  coord         color
        {{-0.5f, -0.5f, 0.0f}, {},    {},     {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {},    {},     {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f, 0.0f}, {},    {},     {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f,  0.5f, 0.0f}, {},    {},     {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, 1.0f}, {},    {},     {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f, 1.0f}, {},    {},     {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f, 1.0f}, {},    {},     {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f,  0.5f, 1.0f}, {},    {},     {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    };
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
    };
    std::string texture = "resources/vulkan.png";
    app->GRAPH_MESHES = {
        {vertices, indices, texture}
    };
#endif

    // set renderer variables
    app->RENDER_CLEAR_VALUES = {0.1f, 0.1f, 0.1f, 1.0f};
    // set camera variables
    app->CAMERA_INIT_POS = glm::vec3(0.0f, 0.0f, 10.0f);
    app->CAMERA_INIT_UP = glm::vec3(0.0f, -1.0f, 0.0f);
    app->CAMERA_SPEED = 1.0f;
    app->StartCamera();

    app->GRAPH_MODEL_PATH = "resources/vulkancube.glb";

    try
    {
        app->StartBackend();
        app->StartRenderer();
        app->LoadGraph();
        app->Loop(update_uniform);
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        ret = APP_EXIT_FAILURE;
    }

    delete app;
    
    return ret;
}