#include "logging.hpp"
#include "files.hpp"

#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>

using namespace LOGGING;
using namespace FILES;

Logger::Logger()
{
    clearAll();
    AddMessage(LOG_OWNERS_LOGGING, "Logging System Started");
}

Logger::~Logger()
{
    clearAll();
}

void Logger::PrintAll()
{
    for(size_t i = 0; i < d_messages.size(); i++)
    {
        std::cout << convertMessage(d_messages[i]) << std::endl;
    }
}

void Logger::PrintByID(LogOwners ID)
{
    for(size_t i = 0; i < d_messages.size(); i++)
    {
        if(d_messages[i].owner == ID)
        {
            std::cout << convertMessage(d_messages[i]) << std::endl;
        }
    }
}

void Logger::DumpToFile(std::string& filePath)
{
    std::stringstream data;
    for(size_t i = 0; i < d_messages.size(); i++)
    {
        data << convertMessage(d_messages[i]) << "\n";
    }
    write_to_file(filePath, data.str());
}

// TODO: store tmp local log before poping
void Logger::AddMessage(LogOwners owner, std::string message, bool print)
{
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[100] = {0};
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    Message newMessage = {owner, message, std::string(buf)};
    while(d_messages.size() >= MAX_SIZE)
    {
        d_messages.pop_front();
    }
    d_messages.push_back(newMessage);
    if(print)
    {
        std::cout << message << std::endl;
    }
}

std::string Logger::convertMessage(Message& message)
{
    std::string owner;
    switch (message.owner)
    {
    case LOG_OWNERS_LOGGING:
        owner = "logger";
        break;
    case LOG_OWNERS_BACKEND:
        owner = "backend";
        break;
    case LOG_OWNERS_RENDERER:
        owner = "renderer";
        break;
    case LOG_OWNERS_USER:
        owner = "user";
        break;
    default:
        owner = "undefined";
        break;
    }
    std::string output = "Log Message (" + message.time + ")[" + owner + "]: " + message.message;
    return output;
}

void Logger::clearAll()
{
    d_messages.clear();
    d_messages.resize(0);
}