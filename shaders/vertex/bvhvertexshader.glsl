#version 450

layout(location = 0) in vec3 aPositions;

uniform mat4 viewProjection;

void main()
{
    gl_Position = viewProjection * vec4(aPositions, 1.0);
}