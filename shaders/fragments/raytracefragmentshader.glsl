#version 450 core

layout(location = 0) in vec2 vTexCoords;
layout(location = 0) out vec4 color;

uniform sampler2D image;

void main()
{
    color = texture(image, vTexCoords);
}