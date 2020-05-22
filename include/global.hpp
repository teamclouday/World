// File Description
// the global application

#pragma once

#include "base.hpp"
#include "logging.hpp"

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
        if(p_backend)
            delete p_backend;
    }

    void StartBackend(){p_backend = new BASE::Backend();}

    LOGGING::Logger* GetLogger(){return p_logger;}
    BASE::Backend* GetBackend(){return p_backend;}

public:
    // parameters for Backend object
    int WINDOW_WIDTH = 800;
    int WINDOW_HEIGHT = 600;
    std::string WINDOW_TITLE = "Hello World";
    bool WINDOW_RESIZABLE = false;
    bool BACKEND_ENABLE_VALIDATION = true;

    // parameters for logger
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
};