// File Description
// the global application

#pragma once

#include "base.hpp"
#include "data.hpp"
#include "camera.hpp"
#include "logging.hpp"

#define APP_EXIT_SUCCESS    0
#define APP_EXIT_FAILURE    1

class Application
{
public:
    Application()
    {
        if(_enable_logger)
            p_logger = new LOGGING::Logger();
    }

    ~Application()
    {
        GRAPH_MESHES.clear();
        GRAPH_MESHES.resize(0);
        if(LOGGER_SAVE_LOG && _enable_logger)
            p_logger->DumpToFile(LOGGER_PATH);
        if(p_logger)
            delete p_logger;
        p_logger = nullptr;
        if(p_camera)
            delete p_camera;
        p_camera = nullptr;
        if(p_renderer)
            delete p_renderer;
        p_renderer = nullptr;
        if(p_backend)
            delete p_backend;
        p_backend = nullptr;
    }

    void StartCamera(){p_camera = new UTILS::Camera(CAMERA_INIT_POS, CAMERA_INIT_UP);}
    void StartBackend(){p_backend = new BASE::Backend();}
    void StartRenderer(){p_renderer = new BASE::Renderer();}
    void LoadGraph(){if(p_renderer) p_renderer->CreateGraph();}
    void Loop(USER_UPDATE user_func){if(p_renderer) p_renderer->loop(user_func);}

    LOGGING::Logger* GetLogger(){return p_logger;}
    BASE::Backend* GetBackend(){return p_backend;}
    BASE::Renderer* GetRenderer(){return p_renderer;}
    UTILS::Camera* GetCamera(){return p_camera;}

public:
    // parameters for Backend
    int WINDOW_WIDTH = 800;
    int WINDOW_HEIGHT = 600;
    std::string WINDOW_TITLE = "Hello World";
    bool WINDOW_RESIZABLE = false;
    bool BACKEND_ENABLE_VALIDATION = true;

    // parameters for renderer creating a graph
    std::vector<DATA::GraphUserInput> GRAPH_MESHES;
    DATA::ShaderSourceDetails GRAPH_SHADER_DETAILS;
    glm::vec4 RENDER_CLEAR_VALUES = {1.0f, 1.0f, 1.0f, 1.0f};
    std::string GRAPH_MODEL_PATH = "";

    // parameters for setting camera
    glm::vec3 CAMERA_INIT_POS = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 CAMERA_INIT_UP = glm::vec3(0.0f, 1.0f, 0.0f);
    float CAMERA_SPEED = 5.0f;

    // parameters for Logger
    std::string LOGGER_PATH = "world.log";
    bool LOGGER_SAVE_LOG = false;

private:
#ifdef IGNORE_LOGGING
    const bool _enable_logger = false;
#else
    const bool _enable_logger = true;
#endif

    LOGGING::Logger* p_logger = nullptr;
    BASE::Backend* p_backend = nullptr;
    BASE::Renderer* p_renderer = nullptr;
    UTILS::Camera* p_camera = nullptr;
};