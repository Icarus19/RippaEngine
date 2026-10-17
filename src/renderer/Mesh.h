#pragma once

#include <cstddef>

class Mesh
{
public:
    Mesh(
        const float* vertices,
        std::size_t vertexByteSize,
        const unsigned int* indices,
        std::size_t indexCount
    );
    ~Mesh();
    
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    void Draw() const;
    
private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;
    
    std::size_t m_indexCount = 0;
};
