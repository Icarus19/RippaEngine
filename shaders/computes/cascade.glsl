#version 450

#include "/includes/raytrace_common.glsl"
#include "/includes/lighting.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba16f, binding = 0) uniform writeonly image3D image;

uniform vec3 gridOrigin;
uniform float spacing, tStart, tEnd;
uniform int tile;
//uniform ivec3 probeCount;
uniform bool isTop;

void main()
{
    ivec3 texel = ivec3(gl_GlobalInvocationID);
    if(any(greaterThanEqual(texel, imageSize(image))))
        return;
    
    ivec3 probe = ivec3(texel.xy / tile, texel.z);
    ivec2 dirTexel = texel.xy % tile;

    vec3 center = gridOrigin + (vec3(probe) + 0.5) * spacing;
    vec3 rd = octDecode((vec2(dirTexel) + 0.5) / float(tile));
    vec3 start = center + rd * tStart;
    float len = tEnd - tStart;

    Hit h;
    bool hit = intersectScene(start, rd, len, false, h);

    vec4 result = vec4(0.0);
    float tLight;
    if(hitLightSphere(start, rd, hit ? h.t : len, tLight))
        result = vec4(LIGHT_COLOR, 1.0);
    else if(hit)
        result = vec4(shadeHit(h, start + rd * h.t, rd), 1.0);
    else if(isTop)
        result = vec4(SKY_COLOR, 1.0);
    
    // One bad ray must never poison the interpolation
    if (any(isnan(result.rgb)) || any(isinf(result.rgb))) result.rgb = vec3(0.0);
    result.rgb = min(result.rgb, vec3(60000.0));

    imageStore(image, texel, result);
    //imageStore(image, texel, vec4(1.0, 0.0, 0.0, 1.0));
}