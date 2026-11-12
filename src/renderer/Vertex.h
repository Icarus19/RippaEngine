#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct GPUVertex
{
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec4 texCoord;
};