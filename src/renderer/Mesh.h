#pragma once

#include <cstddef>
#include <vector>

#include "Texture.h"
#include "Vertex.h"

class Mesh
{
public:
    Mesh(
        std::vector<Vertex> vertices,
        std::vector<unsigned int> indices
        //std::vector<Texture> texture
    );
    
    static Mesh CreatePlane();
    static Mesh CreateCube();
    
    ~Mesh();
    
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    
    void BindRaytracingBuffers(
        unsigned int vertexBinding = 0,
        unsigned int indexBinding = 1
    ) const;
    unsigned int GetVBO() const { return m_vbo; }
    unsigned int GetEBO() const { return m_ebo; }
    
    std::vector<Vertex> GetVertices() const { return m_vertices; }
    std::vector<unsigned int> GetIndices() const { return m_indices; }
    
    void Draw() const;
    
private:
    // Mesh data
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<Texture> m_textures;
    
    // Render data
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    unsigned int m_raytracingVBO = 0;
    
    std::size_t m_indexCount = 0;
    
    void SetupMesh();
};
