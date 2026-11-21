#pragma once

#include <glm/fwd.hpp>

#include "Shader.h"

// TODO: make into class
class Program
{
public:
    Program();
    ~Program();
    
    Program(Program&& other) noexcept;
    Program& operator=(Program&& other) noexcept;
    
    void AttachShader(const Shader& shader);
    void Link();
    void Use();
    
    unsigned int GetId() const { return m_id; }
    
    unsigned int GetUniformLocation(const char* name) const;
    
    // Naive to think these would work I guess
    void SetInt(    unsigned int location, int value);
    void SetFloat(  unsigned int location, float value);
    void SetIVec2(  unsigned int location, int count, const glm::ivec2& value);
    void SetIVec3(  unsigned int location, int count, const glm::ivec3& value);
    void SetIVec4(  unsigned int location, int count, const glm::ivec4& value);
    void SetVec2(   unsigned int location, int count, glm::vec2 value);
    void SetVec3(   unsigned int location, int count, glm::vec3 value);
    void SetVec4(   unsigned int location, int count, glm::vec4 value);
    void SetMat4(   unsigned int location, glm::mat4 value);
    

    
private:
    unsigned int m_id;
    bool m_linked = false;
    
    void CheckLinkErrors() const;
};
