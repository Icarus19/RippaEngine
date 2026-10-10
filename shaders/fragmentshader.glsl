#version 450 core

layout(location = 0) out vec4 color;

in vec3 Color;
in vec2 TexCoord;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float zoom;

void main()
{
    vec2 flippedTexCoord = vec2(-TexCoord.x, TexCoord.y);

    color = mix(texture(texture0, TexCoord), texture(texture1, flippedTexCoord), zoom);
}