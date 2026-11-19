#include "ShaderLibrary.h"

#include <filesystem>
#include <fstream>
#include <iostream>

std::string ShaderLibrary::m_helperSource = "";

void ShaderLibrary::Init(const char* path)
{
    m_helperSource = LoadShaderSource(path);
}


std::string ShaderLibrary::LoadShaderSource(const char* path)
{
    std::cout << "Working directory: "
          << std::filesystem::current_path() << '\n';

    std::cout << "Resolved path: "
              << std::filesystem::absolute(path) << '\n';
    
    std::ifstream file(path);
    
    if (!file)
    {
        std::cerr << "Could not open file " << path << std::endl;
        throw std::runtime_error("Failed to load shader source");
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return buffer.str();
}