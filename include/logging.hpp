// File Description
// Log helper functions

#pragma once

#include <deque>
#include <string>

namespace LOGGING
{
    // set id for each message owner
    enum LogOwners
    {
        LOG_OWNERS_LOGGING  = 0,
        LOG_OWNERS_BACKEND  = 1,
        LOG_OWNERS_RENDERER = 2,
        LOG_OWNERS_GRAPH    = 3,
        LOG_OWNERS_UI       = 4,
        LOG_OWNERS_USER     = 5,
    };

    // structure for each message
    struct Message
    {
        LogOwners owner;
        std::string message;
        std::string time;
    };

    // a runtime logger
    class Logger
    {
    public:
        Logger();
        ~Logger();
        // add a new message
        void AddMessage(LogOwners owner, std::string message, bool print=false);
        // print all messages to console
        void PrintAll();
        // print messages by owner ID
        void PrintByID(LogOwners ID);
        // dump all messages to file
        void DumpToFile(std::string& filePath);

    private:
        // clear all messages
        void clearAll();
        // convert messages to string
        std::string convertMessage(Message& message);

    private:
        const uint32_t MAX_SIZE = 2000u;
        std::deque<Message> d_messages;

    };
}