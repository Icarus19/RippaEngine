#version 450 core

layout(location = 0) in vec3 aPositions;
layout(location = 1) in vec3 aNormals;
layout(location = 2) in vec2 aTexCoords;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.www, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

out vec3 Color;
out vec2 TexCoord;
uniform float time;
uniform mat4 transform;
uniform bool move;

void main()
{
    float hue = float(gl_VertexID) / 4.0;

    Color = hsv2rgb(vec3(hue, 1.0, 1.0));
    //Color = aNormals;
    TexCoord = aTexCoords;
    if(!move)
        gl_Position = transform * vec4(aPositions.x + sin(time), aPositions.y, aPositions.z, 1.0);
    else
        gl_Position = transform * vec4(aPositions, 1.0);
}