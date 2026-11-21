#include "Program.h"

#include <iostream>


Program::Program()
    : m_id(glCreateProgram())
{
    if (m_id == 0)
    {
        throw std::runtime_error("Failed to create a new program");
    }
}

Program::~Program()
{
    if (m_id != 0)
    {
        glDeleteProgram(m_id);
    }
}
/*
Program::Program(Program&& other) noexcept
    : m_id(std::exchange(other.m_id, 0)),
      m_linked(std::exchange(other.m_linked, false))
{
}

Program::Program(Program&& other) noexcept
    : m_id(std::exchange(other.m_id, 0)),
      m_linked(std::exchange(other.m_linked, false))
{
    if (this != &other)
    {
        if (m_id != 0)
        {
            glDeleteProgram(m_id);
        }
        
        m_id = std::exchange(other.m_id, 0);
        m_linked = std::exchange(other.m_linked, false);
    }
    
    return *this;
}
*/
void Program::AttachShader(const Shader& shader)
{
    if (m_linked)
        throw std::runtime_error("Cannot attach shader after linking");
    
    glAttachShader(m_id, shader.GetId());
}

void Program::Link()
{
    glLinkProgram(m_id);
    CheckLinkErrors();
    m_linked = true;
}

void Program::Use()
{
    if (!m_linked)
        throw std::runtime_error("Cannot use unlinked shader program");
    
    glUseProgram(m_id);
}

unsigned int Program::GetUniformLocation(const char* name) const
{
    unsigned int location = glGetUniformLocation(m_id, name);
    
    if (location == -1)
        std::cerr << "Warning: uniform location not found: " << name << std::endl;
    
    return location;
}

void Program::CheckLinkErrors() const
{
    GLint success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    
    if (!success)
    {
        GLint length;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &length);

        std::string infoLog(length, '\0');

        glGetProgramInfoLog(
            m_id,
            length,
            nullptr,
            infoLog.data()
        );

        std::cerr << "Shader Program Linking Failed:\n"
                  << infoLog << std::endl;

        throw std::runtime_error(
            "Failed to link shader program!");
    }
}