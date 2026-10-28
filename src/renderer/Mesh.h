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
    
    static Mesh CreateCube();
    
    ~Mesh();
    
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    
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
    
    std::size_t m_indexCount = 0;
    
    void SetupMesh();
};
