#version 450

#include "/includes/raytrace_common.glsl"
#include "/includes/lighting.glsl"

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba16f, binding = 0) uniform writeonly image2D image;

uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform sampler3D cascade0;
uniform vec3 gridOrigin;
uniform float spacing0;
uniform ivec3 probeCount0;
uniform int tile0;


vec3 probeIrradiance(ivec3 p, vec3 n)
{
    p = clamp(p, ivec3(0), probeCount0 - 1);
    vec3 sum = vec3(0.0);
    int dirCount = tile0 * tile0;
    for (int y = 0; y < tile0; y++)
        for (int x = 0; x < tile0; x++)
        {
            vec3 d = octDecode((vec2(x, y) + 0.5) / float(tile0));
            sum += texelFetch(cascade0, ivec3(p.xy * tile0 + ivec2(x, y), p.z), 0).rgb * max(dot(n, d), 0.0);
        }
    return sum * (4.0 / float(dirCount));
}

vec3 sampleIndirect(vec3 pos, vec3 n)
{
    vec3 g = (pos + n * spacing0 * 0.5 - gridOrigin) / spacing0 - 0.5;   // nudge off the surface
    ivec3 g0 = ivec3(floor(g));
    vec3 f = g - vec3(g0);
    vec3 acc = vec3(0.0);
    for (int k = 0; k < 8; k++)
    {
        ivec3 o = ivec3(k & 1, (k >> 1) & 1, (k >> 2) & 1);
        vec3 wv = mix(1.0 - f, f, vec3(o));
        acc += wv.x * wv.y * wv.z * probeIrradiance(g0 + o, n);
    }
    return acc;
}

void main()
{
    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(image);
    if(pix.x >= size.x || pix.y >= size.y)
        return;

    // Camera ray
    vec2 ndc = ((vec2(pix) + 0.5) / vec2(size)) * 2.0 - 1.0;
    vec4 p = inverseProjection * vec4(ndc, 1.0, 1.0);
    vec3 dir = normalize((inverseView * vec4(normalize(p.xyz / p.w), 0.0)).xyz);
    vec3 origin = (inverseView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  
    Hit h;
    bool hit = intersectScene(origin, dir, 1e30, false, h);

    float tLight;
    if(hitLightSphere(origin, dir, hit ? h.t : 1e30, tLight))
    {
        imageStore(image, pix, vec4(LIGHT_COLOR, 1.0));
        return;
    }
    if(!hit)
    {
        imageStore(image, pix, vec4(SKY_COLOR, 1.0));
        return;
    }

    Vertex a = vertices[indices[3u*h.tri + 0u]];
    Vertex b = vertices[indices[3u*h.tri + 1u]];
    Vertex c = vertices[indices[3u*h.tri + 2u]];
    float w = 1.0 - h.u - h.v;
    GPUMaterial mat = materials[triMaterials[h.tri]];
    float lod = log2(max(h.t * 0.25, 1.0));

    vec2 uv = w * a.texCoord.xy + h.u * b.texCoord.xy + h.v * c.texCoord.xy;
    vec3 dp1 = b.position.xyz - a.position.xyz;
    vec3 dp2 = c.position.xyz - a.position.xyz;
    vec3 ng = normalize(cross(dp1, dp2));
    vec3 n = w * a.normal.xyz + h.u * b.normal.xyz + h.v * c.normal.xyz;
    n = (dot(n, n) < 1e-8) ? ng : normalize(n);
    if(dot(ng, dir) > 0.0) { n = -n; ng = -ng; }

    if(hasTexture(mat.normalTexture))
    {
        vec2 duv1 = b.texCoord.xy - a.texCoord.xy;
        vec2 duv2 = c.texCoord.xy - a.texCoord.xy;
        float det = duv1.x * duv2.y - duv1.y * duv2.x;
        if(abs(det) > 1e-12)
        {
            vec3 T = normalize((dp1 * duv2.y - dp2 * duv1.y) / det);
            T = normalize(T - n * dot(n, T));
            vec3 B = cross(n, T);
            vec3 tn = textureLod(sampler2D(mat.normalTexture), uv, lod).rgb * 2.0 - 1.0;
            n = normalize(T * tn.x + B * tn.y + n * tn.z);
        }
    }

    vec3 albedo = mat.albedo;
    if(mat.albedoTexture != uvec2(0))
        albedo *= textureLod(sampler2D(mat.albedoTexture), uv, lod).rgb;

	/*
    float specStrength = 0.0;
    if(hasTexture(mat.specularTexture))
        specStrength = textureLod(sampler2D(mat.specularTexture), uv, lod).r;

    float roughness = clamp(mat.roughness, 0.05, 1.0);
	*/

    vec3 position = origin + dir * h.t;
    vec3 direct = directLighting(position, n, ng);
    vec3 indirect = sampleIndirect(position, n);

    vec3 result = albedo * (direct + indirect);
    if (pix.x < imageSize(image).x / 2)
    {
        if(pix.y < imageSize(image).y / 2)
            result = direct;
        else
            result = albedo;
    }   
    else
    {
        if(pix.y < imageSize(image).y / 2)
            result = indirect;
    }
    
    imageStore(image, pix, vec4(result, 1.0));
}