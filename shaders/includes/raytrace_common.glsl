#extension GL_ARB_bindless_texture : require

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
struct Hit {
    float t;
    uint tri;
    float u, v;
};

layout(std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };
layout(std430, binding = 1) readonly buffer IndexBuffer { uint indices[]; };
layout(std430, binding = 2) readonly buffer NodeBuffer { BVHNode nodes[]; };
layout(std430, binding = 3) readonly buffer TriMaterials { uint triMaterials[]; };
layout(std430, binding = 4) readonly buffer Materials { GPUMaterial materials[]; };


bool hasTexture(uvec2 h) 
{ 
    return h.x != 0u || h.y != 0u; 
}


float intersectAABB(vec3 ro, vec3 invDir, vec3 bmin, vec3 bmax, float tMax)
{
    vec3 t0 = (bmin - ro) * invDir;
    vec3 t1 = (bmax - ro) * invDir;
    vec3 tNearV = min(t0, t1);
    vec3 tFarV = max(t0, t1);
    float tNear = max(max(tNearV.x, tNearV.y), max(tNearV.z, 0.0));
    float tFar = min(min(tFarV.x, tFarV.y), tFarV.z);
    return (tNear <= tFar && tNear < tMax) ? tNear : 1e30;
}

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