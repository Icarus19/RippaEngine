#pragma once

#include <glad/glad.h>
#include <string>

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

    static std::string LoadShaderSource(const char* path);
};
