#pragma once
#include <vector>
#include <glm/vec3.hpp>

#include "Vertex.h"

struct BVHNode
{
    glm::vec3 boundsMin; uint32_t leftFirst;
    glm::vec3 boundsMax; uint32_t triCount;
};
static_assert(sizeof(BVHNode) == 32, "Enforce std430 layout");

class BVH
{
public:
    BVH(const std::vector<GPUVertex>& vertices, const std::vector<unsigned int>& indices);
    
    ~BVH();
    
    const std::vector<BVHNode>& GetNodes() const { return m_nodes; }
    const std::vector<unsigned int>& GetIndices() const { return m_indices; }
    std::vector<glm::vec3> GetBoxLines(uint32_t depth) const;
    
private:
    struct TriInfo
    {
        glm::vec3 min, max, centroid;
    };
    
    static constexpr uint32_t MaxLeafTris = 4;
    
    std::vector<TriInfo> m_tris;
    std::vector<uint32_t> m_order;
    std::vector<BVHNode> m_nodes;
    std::vector<unsigned int> m_indices;
    std::vector<uint32_t> m_depths;
    
    uint32_t Build(uint32_t begin, uint32_t end, uint32_t depth);
};
