// File Description
// the global application

#pragma once

#include "base.hpp"
#include "data.hpp"
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
        if(LOGGER_SAVE_LOG && _enable_logger)
            p_logger->DumpToFile(LOGGER_PATH);
        if(p_logger)
            delete p_logger;
        p_logger = nullptr;
        if(p_renderer)
            delete p_renderer;
        p_renderer = nullptr;
        if(p_backend)
            delete p_backend;
        p_backend = nullptr;
    }

    void StartBackend(){p_backend = new BASE::Backend();}
    void StartRenderer(){p_renderer = new BASE::Renderer();}

    LOGGING::Logger* GetLogger(){return p_logger;}
    BASE::Backend* GetBackend(){return p_backend;}
    BASE::Renderer* GetRenderer(){return p_renderer;}

public:
    // parameters for Backend
    int WINDOW_WIDTH = 800;
    int WINDOW_HEIGHT = 600;
    std::string WINDOW_TITLE = "Hello World";
    bool WINDOW_RESIZABLE = false;
    bool BACKEND_ENABLE_VALIDATION = true;

    // parameters for renderer creating a graph
    std::vector<DATA::MeshInput> GRAPH_MESHES;
    DATA::ShaderSourceDetails GRAPH_SHADER_DETAILS;

    // parameters for a graph
    float RENDER_CLEAR_VALUES[4] = {1.0f, 1.0f, 1.0f, 1.0f};

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
};