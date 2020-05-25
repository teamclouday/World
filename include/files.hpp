// File Description
// A file system helper
// read, write files

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept>

namespace FILES
{
// define the global file folder
// for easy file loading
// TODO: change to load files in release mode
#ifndef GLOB_FILE_FOLDER
#define GLOB_FILE_FOLDER "."
#endif

    // write string of data to a file
    static void write_to_file(std::string& filePath, std::string& data)
    {
        std::string path = std::string(GLOB_FILE_FOLDER) + "/" + filePath;
        std::ofstream outFile(path.c_str(), std::ios::out | std::ios::trunc);
        if(!outFile.is_open())
        {
            std::string message = "failed to open file for writing: " + path;
            throw std::runtime_error(message);
        }
        outFile << data;
        outFile.close();
    }

    // read string from a file
    static std::string read_string_from_file(std::string& filePath)
    {
        std::string path = std::string(GLOB_FILE_FOLDER) + "/" + filePath;
        std::ifstream inFile(path.c_str(), std::ios::binary);
        if(!inFile.is_open())
        {
            std::string message = "failed to open file for reading: " + path;
            throw std::runtime_error(message);
        }
        std::stringstream data;
        char chr = inFile.get();
        while(inFile.good())
        {
            data << chr;
            chr = inFile.get();
        }
        inFile.close();
        return data.str();
    }

    // read bytes from a file
    static std::vector<char> read_bytes_from_file(std::string& filePath)
    {
        std::string path = std::string(GLOB_FILE_FOLDER) + "/" + filePath;
        std::ifstream inFile(path.c_str(), std::ios::ate | std::ios::binary);
        if(!inFile.is_open())
        {
            std::string message = "failed to open file for reading: " + path;
            throw std::runtime_error(message);
        }
        size_t fileSize = (size_t)inFile.tellg();
	    std::vector<char> buffer(fileSize);

	    inFile.seekg(0);
	    inFile.read(buffer.data(), fileSize);
	    inFile.close();

	    return buffer;
    }

    // get file extension
    static std::string get_file_extension(const std::string filePath)
    {
        std::size_t idx = filePath.find_last_of(".");
        if(idx == std::string::npos)
            return "";
        std::string ext = filePath.substr(idx+1);
        return ext;
    }
}