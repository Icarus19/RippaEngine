#version 450 core

layout(location = 0) in vec2 vTexCoords;
layout(location = 0) out vec4 color;

uniform sampler2D albedoMap;
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

uniform float time;

struct Hit {
    float t;
    uint tri;
    float u, v;
};

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
                    h.t = t;
                    h.tri = i;
                    h.u = u;
                    h.v = v;                    
                    hit = true;
                    if(anyHit) return true;
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

    vec3 n = normalize(w * a.normal.xyz + h.u * b.normal.xyz + h.v * c.normal.xyz);
    vec2 uv = w * a.texCoord.xy + h.u * b.texCoord.xy + h.v * c.texCoord.xy;
    if(dot(n, dir) > 0.0) n = -n;

    vec3 position = origin + dir * h.t;
    vec3 albedo = textureLod(albedoMap, uv, 0.0).rgb;

    // Light only exists on the shader for now
    // Directional light
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

    vec3 lit = shadowed ? vec3(0.0) : vec3(0.87) * ndotl / attenuation;
    vec3 ambient = vec3(0.05);

    color = vec4(albedo * (ambient + lit), 1.0);
    //float pulse = abs(sin(time));
    //color = vec4(pulse, 0.0, 0.0, 1.0);
}