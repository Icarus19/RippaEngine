#version 450 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoords;

layout(location = 0) out vec2 vTexCoords;

void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vTexCoords = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}
