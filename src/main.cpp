#include "global.hpp"

Application* app = new Application();

#include <iostream>

// user defined camera uniform update function
void update_uniform(DATA::CameraUniform& data, uint32_t width, uint32_t height)
{
    data.view = app->GetCamera()->GetViewMatrix();
    data.proj = glm::perspective(glm::radians(60.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 1000.0f);
    data.model = glm::scale(glm::mat4(1.0f), glm::vec3(app->GetCamera()->mv_zoom));
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
        {{-0.5f, -0.5f, 0.0f}, {},    {},     {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f, 0.0f}, {},    {},     {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f, 0.0f}, {},    {},     {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f,  0.5f, 0.0f}, {},    {},     {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, 1.0f}, {},    {},     {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f, 1.0f}, {},    {},     {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f, 1.0f}, {},    {},     {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f,  0.5f, 1.0f}, {},    {},     {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
    };
    std::vector<uint32_t> indices = {
        4, 5, 6, 6, 7, 4,
        0, 1, 2, 2, 3, 0,
    };
    std::string texture = "resources/vulkan.png";
    app->GRAPH_MESHES = {
        {vertices, indices, texture}
    };
#endif

    // set renderer variables
    app->RENDER_CLEAR_VALUES = {0.1f, 0.1f, 0.1f, 1.0f};
    app->RENDER_ENABLE_DEPTH = true;
    app->RENDER_ENABLE_MSAA = false;
    // set camera variables
    app->CAMERA_INIT_POS = glm::vec3(0.0f, 0.0f, 10.0f);
    app->CAMERA_ZOOM_SCALE = 0.01f;
    app->CAMERA_SPEED = 5.0f;
    app->StartCamera();

    app->GRAPH_MODEL_PATH = "resources/DamagedHelmet.gltf";

    try
    {
        app->StartBackend();
        app->StartRenderer();
        app->StartUI();
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