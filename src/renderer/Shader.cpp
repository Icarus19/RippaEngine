#include "Shader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* path, GLenum type)
{
    std::string shaderSource = LoadShaderSource(path);
    const char* shaderPointer = shaderSource.c_str();
    
    m_id = glCreateShader(type);
    m_type = type;
    
    glShaderSource(m_id, 1, &shaderPointer, NULL);
    glCompileShader(m_id);
    
    int success;
    char infoLog[512];

    glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(m_id, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n"
                  << infoLog << '\n';
    }
}

Shader::~Shader()
{
    glDeleteShader(m_id);
}

unsigned int Shader::GetId() const
{
    return m_id;
}

GLenum Shader::GetType() const
{
    return m_type;
}

std::string Shader::LoadShaderSource(const char* path)
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