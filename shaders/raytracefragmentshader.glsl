#version 450 core

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

layout(std430, binding = 0) readonly buffer VertexBuffer { Vertex vertices[]; };
layout(std430, binding = 1) readonly buffer IndexBuffer { uint indices[]; };
layout(std430, binding = 2) readonly buffer NodeBuffer { BVHNode nodes[]; };

uniform uint triCount;
// ro is Ray origin, and rd is Ray Direction
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

bool intersectScene(vec3 ro, vec3 rd, out float tBest, out vec3 nBest)
{
    tBest = 1e30;
    nBest = vec3(0.0);
    bool hit = false;

    vec3 invDir = 1.0 / rd;

    uint stack[64];
    int sp = 0;
    stack[sp++] = 0u;

    while(sp > 0)
    {
        uint idx = stack[--sp];
        BVHNode node = nodes[idx];

        if(intersectAABB(ro, invDir, node.boundsMin, node.boundsMax, tBest) >= 1e30)
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
                if(v < 0.0 || v > 1.0) continue;

                float t = dot(e2, qv) * invDet;
                if(t > 1e-4 && t < tBest)
                {
                    tBest = t;
                    nBest = normalize(cross(e1, e2));
                    hit = true;
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
    
    float t;
    vec3 n;
    if(intersectScene(origin, dir, t, n))
        color = vec4(n * 0.5 + 0.5, 1.0);
    else
        color = vec4(0.0, 0.0, 0.0, 1.0);
}