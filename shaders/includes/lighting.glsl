// ---- Dummy scene data. Replace with real values later ----
const vec3  SKY_COLOR    = vec3(0.35, 0.5, 0.8);
const vec3  SUN_DIR      = normalize(vec3(0.3, -1.0, 0.2));   // direction the light travels
const vec3  SUN_COLOR    = vec3(0.87);
const vec3  LIGHT_POS[4] = vec3[4](vec3(-5.0, 2.0, -2.0), vec3(5.0, 2.0, -2.0),
                                   vec3(-5.0, 2.0,  2.0), vec3(5.0, 2.0,  2.0));
const float LIGHT_RADIUS = 0.15;
const vec3  LIGHT_COLOR  = vec3(5.0, 3.0, 2.0);               // warm, bright

vec3 octDecode(vec2 e)
{
    e = e * 2.0 - 1.0;
    vec3 v = vec3(e, 1.0 - abs(e.x) - abs(e.y));
    if(v.z < 0.0) v.xy = (1.0 - abs(v.yx)) * vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
    return normalize(v);
}

bool hitLightSphere(vec3 ro, vec3 rd, float tMax, out float tOut)
{
    tOut = tMax;
    bool hit = false;
    for(int i = 0; i < 4; i++)
    {
        vec3 oc = ro - LIGHT_POS[i];
        float b = dot(oc, rd);
        float disc = b*b - (dot(oc, oc) - LIGHT_RADIUS * LIGHT_RADIUS);
        if(disc <= 0.0) continue;
        float t = -b - sqrt(disc);
        if(t > 0.0 && t < tOut) { tOut = t; hit = true; }
    }
    
    return hit;
}

vec3 directLighting(vec3 pos, vec3 n, vec3 ng)
{
    vec3 lit = vec3(0.0);
    vec3 o = pos + ng * 1e-3;
    Hit sh;

    float nl = max(dot(n, -SUN_DIR), 0.0);
    if (nl > 0.0 && !intersectScene(o, -SUN_DIR, 1e4, true, sh))
        lit += SUN_COLOR * nl;

    for (int i = 0; i < 4; i++)
    {
        vec3 toL = LIGHT_POS[i] - pos;
        float d = length(toL);
        vec3 L = toL / d;
        float ndl = max(dot(n, L), 0.0);
        if (ndl <= 0.0 || intersectScene(o, L, d, true, sh)) continue;
        lit += LIGHT_COLOR * ndl / (1.0 + 0.1*d + 0.02*d*d);
    }
    return lit;
}

vec3 shadeHit(Hit h, vec3 pos, vec3 rd)
{
    Vertex a = vertices[indices[3u*h.tri + 0u]];
    Vertex b = vertices[indices[3u*h.tri + 1u]];
    Vertex c = vertices[indices[3u*h.tri + 2u]];
    float w = 1.0 - h.u - h.v;

    vec3 ng = normalize(cross(b.position.xyz - a.position.xyz, c.position.xyz - a.position.xyz));
    if (dot(ng, rd) > 0.0) ng = -ng;

    vec2 uv = w*a.texCoord.xy + h.u*b.texCoord.xy + h.v*c.texCoord.xy;
    GPUMaterial m = materials[triMaterials[h.tri]];
    vec3 albedo = m.albedo;
    if (hasTexture(m.albedoTexture))
        albedo *= textureLod(sampler2D(m.albedoTexture), uv, 3.0).rgb;

    return m.emission + albedo * directLighting(pos, ng, ng);
}