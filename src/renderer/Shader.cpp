#include "Shader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <assets/ShaderLibrary.h>


// TODO: Parse strings for #include files and only include asked for classes
Shader::Shader(const char* path, GLenum type)
{
    ShaderSource shaderSource = ShaderLibrary::LoadShaderSource(path);
    
    const char* shaderPointer = shaderSource.source.c_str();
    
    m_id = glCreateShader(type);
    m_type = type;
    
    glShaderSource(m_id, 1, &shaderPointer, NULL);
    glCompileShader(m_id);
    
    int success;
    int logLength;

    glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);
    glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &logLength);

    if (!success)
    {
        std::string infoLog(logLength, '\0');
        glGetShaderInfoLog(m_id, logLength, nullptr, infoLog.data());
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