#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>
#include <filesystem>


struct ShaderSource
{
    std::string source;
    std::vector<std::filesystem::path> sourceFiles;
};

class Shader
{
public:
    Shader(const char* path, GLenum type);
    ~Shader();
    
    unsigned int GetId() const;
    GLenum GetType() const;
    
private:
    unsigned int m_id;
    GLenum m_type;
    ShaderSource m_shaderSource;
};
