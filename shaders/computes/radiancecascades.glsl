#version 450
#extension GL_ARB_bindless_texture : require

#include "/includes/common.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    int test = 0;
}