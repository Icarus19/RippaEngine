#include "BVH.h"

#include <algorithm>
#include <glm/common.hpp>

BVH::BVH(const std::vector<GPUVertex>& vertices, const std::vector<unsigned int>& indices)
{
    const uint32_t triCount = (uint32_t)indices.size() / 3;
    
    m_tris.resize(triCount);
    m_order.resize(triCount);
    for (uint32_t i = 0; i < triCount; i++)
    {
        glm::vec3 a = glm::vec3(vertices[indices[3*i + 0]].position);
        glm::vec3 b = glm::vec3(vertices[indices[3*i + 1]].position);
        glm::vec3 c = glm::vec3(vertices[indices[3*i + 2]].position);
        m_tris[i].min = glm::min(a, glm::min(b, c));
        m_tris[i].max = glm::max(a, glm::max(b, c));
        m_tris[i].centroid = (a + b + c) / 3.0f;
        m_order[i] = i;
    }
    
    m_nodes.reserve(2 * triCount);
    if (triCount > 0)
        Build(0, triCount, 0);
    
    m_indices.resize(indices.size());
    for (uint32_t i = 0; i < triCount; i++)
    {
        uint32_t src = m_order[i];
        m_indices[3*i + 0] = indices[3*src + 0];
        m_indices[3*i + 1] = indices[3*src + 1];
        m_indices[3*i + 2] = indices[3*src + 2];
    }
}

BVH::~BVH()
{
    // Don't know what to delete here
}

std::vector<glm::vec3> BVH::GetBoxLines(uint32_t depth) const
{
    std::vector<glm::vec3> lines;
    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        const BVHNode& n = m_nodes[i];
        bool isLeaf = n.triCount > 0;
        
        // Make sure not to go too deep
        if (m_depths[i] != depth && !(isLeaf && m_depths[i] < depth)) continue;
        
        glm::vec3 lo = n.boundsMin, hi = n.boundsMax;
        glm::vec3 c[8] = {
            {lo.x,lo.y,lo.z}, {hi.x,lo.y,lo.z}, {hi.x,hi.y,lo.z}, {lo.x,hi.y,lo.z},
            {lo.x,lo.y,hi.z}, {hi.x,lo.y,hi.z}, {hi.x,hi.y,hi.z}, {lo.x,hi.y,hi.z}
        };
        const int e[12][2] = {
            {0,1},{1,2},{2,3},{3,0},   // back face
            {4,5},{5,6},{6,7},{7,4},   // front face
            {0,4},{1,5},{2,6},{3,7}    // connecting edges
        };
        for (auto& edge : e) { lines.push_back(c[edge[0]]); lines.push_back(c[edge[1]]); }
    }
    
    return lines;
}

uint32_t BVH::Build(uint32_t begin, uint32_t end, uint32_t depth)
{
    const uint32_t nodeIdx = (uint32_t)m_nodes.size();
    m_nodes.emplace_back();
    m_depths.push_back(depth);
    
    glm::vec3 bmin(1e30f), bmax(-1e30f);
    glm::vec3 cmin(1e30f), cmax(-1e30f);
    for (uint32_t i = begin; i < end; i++)
    {
        const TriInfo& t = m_tris[m_order[i]];
        bmin = glm::min(bmin, t.min);
        bmax = glm::max(bmax, t.max);
        cmin = glm::min(cmin, t.centroid);
        cmax = glm::max(cmax, t.centroid);
    }
    
    const uint32_t count = end - begin;
    if (count <= MaxLeafTris)
    {
        m_nodes[nodeIdx] = { bmin, begin, bmax, count };
        return nodeIdx;
    }
    
    // Split at the longer axis
    glm::vec3 ext = cmax - cmin;
    int axis = (ext.y > ext.x) ? 1 : 0;
    if (ext.z > ext[axis]) axis = 2;

    const uint32_t mid = begin + count / 2;
    std::nth_element(
        m_order.begin() + begin, m_order.begin() + mid, m_order.begin() + end,
        [&](uint32_t a, uint32_t b){
            return m_tris[a].centroid[axis] < m_tris[b].centroid[axis];
        }
    );
    
    Build(begin, mid, depth + 1);
    uint32_t right = Build(mid, end, depth + 1);
    
    m_nodes[nodeIdx] = { bmin, right, bmax, 0 };
    return nodeIdx;
}
