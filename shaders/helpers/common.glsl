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