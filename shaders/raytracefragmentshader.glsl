#version 450 core
#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec2 vTexCoords;
layout(location = 0) out vec4 color;

uniform mat4 inverseProjection;
uniform mat4 inverseView;

struct Vertex {
    vec4 position;
    vec4 normal;
    vec4 texCoord;
};

struct BVHNode {
    vec3 boundsMin; uint leftFirst;
    vec3 boundsMax; uint triCount;
};

struct GPUMaterial {
    vec3 albedo; float roughness;
    vec3 emission; float metallic;
    uvec2 albedoTexture; uvec2 normalTexture;
    uvec2 specularTexture; uvec2 alphaTexture;
};

layout(std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };
layout(std430, binding = 1) readonly buffer IndexBuffer { uint indices[]; };
layout(std430, binding = 2) readonly buffer NodeBuffer { BVHNode nodes[]; };
layout(std430, binding = 3) readonly buffer TriMaterials { uint triMaterials[]; };
layout(std430, binding = 4) readonly buffer Materials { GPUMaterial materials[]; };

uniform float time;

struct Hit {
    float t;
    uint tri;
    float u, v;
};

bool hasTexture(uvec2 h) { return h.x != 0u || h.y != 0u; }

// ro is Ray origin, and rd is Ray Direction
float intersectAABB(vec3 ro, vec3 invDir, vec3 bmin, vec3 bmax, float tMax);

bool intersectScene(vec3 ro, vec3 rd, float tMax, bool anyHit, out Hit h)
{
    h.t = tMax;
    h.tri = 0u;
    h.u = 0.0;
    h.v = 0.0;
    bool hit = false;

    vec3 invDir = 1.0 / rd;
    uint stack[64];
    int sp = 0;
    stack[sp++] = 0u;

    while(sp > 0)
    {
        uint idx = stack[--sp];
        BVHNode node = nodes[idx];

        if(intersectAABB(ro, invDir, node.boundsMin, node.boundsMax, h.t) >= 1e30)
            continue;

        if(node.triCount > 0u)
        {
            for(uint i = node.leftFirst; i < node.leftFirst + node.triCount; i++)
            {
                vec3 p0 = vertices[indices[3u*i + 0u]].position.xyz;
                vec3 p1 = vertices[indices[3u*i + 1u]].position.xyz;
                vec3 p2 = vertices[indices[3u*i + 2u]].position.xyz;

                vec3 e1 = p1 - p0;
                vec3 e2 = p2 - p0;
                vec3 pv = cross(rd, e2);
                float det = dot(e1, pv);
                if(abs(det) < 1e-8) continue; // Parallel

                float invDet = 1.0 / det;
                vec3 tv = ro - p0;
                float u = dot(tv, pv) * invDet;
                if(u < 0.0 || u > 1.0) continue;

                vec3 qv = cross(tv, e1);
                float v = dot(rd, qv) * invDet;
                if(v < 0.0 || u + v > 1.0) continue;

                float t = dot(e2, qv) * invDet;
                if(t > 1e-4 && t < h.t)
                {
                    GPUMaterial m = materials[triMaterials[i]];
                    if (hasTexture(m.alphaTexture))
                    {
                        float w = 1.0 - u - v;
                        vec2 uv = w * vertices[indices[3u*i + 0u]].texCoord.xy
                                + u * vertices[indices[3u*i + 1u]].texCoord.xy
                                + v * vertices[indices[3u*i + 2u]].texCoord.xy;
                        if (textureLod(sampler2D(m.alphaTexture), uv, 0.0).r < 0.5) continue;
                    }
                    h.t = t; h.tri = i; h.u = u; h.v = v;
                    hit = true;
                    if (anyHit) return true;
                }
            }
        }
        else
        {
            stack[sp++] = node.leftFirst;
            stack[sp++] = idx + 1u;
        }
    }

    return hit;
}

void main()
{
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
    color = vec4(albedo, 1.0);
    //float pulse = abs(sin(time));
    //color = vec4(pulse, 0.0, 0.0, 1.0);
}