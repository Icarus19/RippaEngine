#include "Shader.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "assets/ShaderLibrary.h"

// TODO: Parse strings for #include files and only include asked for classes
Shader::Shader(const char* path, GLenum type)
{
    std::string shaderSource = ShaderLibrary::LoadShaderSource(path);
    const std::string& shaderHelper = ShaderLibrary::GetSource();
    const char* shaderPointer[] = {
        shaderSource.c_str(),
        shaderHelper.c_str()
    };
    
    m_id = glCreateShader(type);
    m_type = type;
    
    glShaderSource(m_id, 2, shaderPointer, NULL);
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