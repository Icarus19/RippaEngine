#version 450

#include "/includes/raytrace_common.glsl"
#include "/includes/lighting.glsl"

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba16f, binding = 0) uniform image2D image;

void main()
{
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);

    ivec2 pix = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(image);
    if(pix.x >= size.x || pix.y >= size.y)
        return;

    vec2 vTexCoords = (vec2(pix) + 0.5) / vec2(size);
    vec2 ndc = vTexCoords * 2.0 - 1.0;
    vec4 p = inverseProjection * vec4(ndc, 1.0, 1.0);
    vec3 dirView = normalize(p.xyz / p.w);
    vec3 dir = normalize((inverseView * vec4(dirView, 0.0)).xyz);
    vec3 origin = (inverseView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    
    Hit h;
    if(!intersectScene(origin, dir, 1e30, false, h))
    {
        color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    Vertex a = vertices[indices[3u*h.tri + 0u]];
    Vertex b = vertices[indices[3u*h.tri + 1u]];
    Vertex c = vertices[indices[3u*h.tri + 2u]];
    float w = 1.0 - h.u - h.v;
    GPUMaterial mat = materials[triMaterials[h.tri]];
    float lod = log2(max(h.t * 0.025, 1.0));

    vec3 dp1 = b.position.xyz - a.position.xyz;
    vec3 dp2 = c.position.xyz - a.position.xyz;
    vec3 ng = normalize(cross(dp1, dp2));
    vec3 n = w * a.normal.xyz + h.u * b.normal.xyz + h.v * c.normal.xyz;
    n = (dot(n, n) < 1e-8) ? ng : normalize(n);
    vec2 uv = w * a.texCoord.xy + h.u * b.texCoord.xy + h.v * c.texCoord.xy;
    if(dot(ng, dir) > 0.0) { n = -n; ng = -ng; }

    if(hasTexture(mat.normalTexture))
    {
        vec2 duv1 = b.texCoord.xy - a.texCoord.xy;
        vec2 duv2 = c.texCoord.xy - a.texCoord.xy;
        float det = duv1.x * duv2.y - duv1.y * duv2.x;
        if(abs(det) > 1e-12)
        {
            float r = 1.0 / det;
            vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * r;
            vec3 B = (dp2 * duv1.y - dp1 * duv2.y) * r;
            T = normalize(T - n * dot(n, T));
            if(dot(cross(n, T), B) < 0.0) B = -cross(n, T); else B = cross(n, T);
            vec3 tn = textureLod(sampler2D(mat.normalTexture), uv, lod).rgb * 2.0 - 1.0;
            n - normalize(T * tn.x + B * tn.y + n * tn.z);
        }
    }

    vec3 albedo = mat.albedo;
    if(mat.albedoTexture != uvec2(0))
        albedo *= textureLod(sampler2D(mat.albedoTexture), uv, lod).rgb;

    float specStrength = 0.0;
    if(hasTexture(mat.specularTexture))
        specStrength = textureLod(sampler2D(mat.specularTexture), uv, lod).r;

    float roughness = clamp(mat.roughness, 0.05, 1.0);

    // Light only exists on the shader for now
    // Directional light
    vec3 position = origin + dir * h.t;
    float angle = time * (6.28318530718 / 60.0);

    vec3 lightDir = normalize(vec3(cos(angle), -0.6, sin(angle)));
    vec3 L = -lightDir;
    float dist = 10000.0;
    float ndotl = max(dot(n, L), 0.0);
    float attenuation = 1.0;
    // Spot light
    /*vec3 toLight = vec3(-2.0, 5.0, -2.0) - position;
    float dist = length(toLight);
    vec3 L = toLight / dist;
    float ndotl = max(dot(n, L), 0.0);
    float attenuation = 1.0 / (1.0 + 0.02 * dist + 0.001 * (dist * dist));
    */
    Hit sh;
    bool shadowed = intersectScene(position + n * 1e-3, L, dist, true, sh);
    float shininess = 2.0 / (roughness * roughness) - 2.0;
    float spec = pow(max(dot(n, L), 0.0), shininess) * specStrength * step(0.0, ndotl);

    vec3 diffuse = albedo * ndotl;
    vec3 lit = shadowed ? vec3(0.0) : vec3(0.87) * (diffuse + spec) / attenuation;
    vec3 ambient = vec3(0.01);

    //color = vec4(albedo * (ambient + lit), 1.0);
    //color = vec4(n * 0.5 + 0.5, 1.0);
    
    imageStore(image, pix, vec4(albedo, 1.0));
    //imageStore(image, pix, vec4(1.0, 0.0, 0.0, 1.0));
}