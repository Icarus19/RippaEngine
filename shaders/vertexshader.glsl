#version 450 core

layout(location = 0) in vec3 position;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.www, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

out vec3 vColor;

void main()
{
    float hue = float(gl_VertexID) / 4.0;

    vColor = hsv2rgb(vec3(hue, 1.0, 1.0));

    gl_Position = vec4(position.x, position.y, position.z, 1.0);
}