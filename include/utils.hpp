// File Description
// utility functions and structures
// 1. shader source related structure

#pragma once

#include <vector>
#include <string>

namespace UTILS
{
    enum ShaderTypes
    {
        SHADER_VERTEX        = 0,
        SHADER_TESS_CONTROL  = 1,
        SHADER_TESS_EVALUATE = 2,
        SHADER_GEOMETRY      = 3,
        SHADER_FRAGMENT      = 4,
        SHADER_COMPUTE       = 5,
    };

    struct ShaderSourceDetails
    {
        std::vector<ShaderTypes> types;
        std::vector<std::string> names;

        bool validate()
        {
            if (!types.size()) return false;
            if (!names.size()) return false;
            return types.size() == names.size();
        }
    };
}