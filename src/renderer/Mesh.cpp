#include "Mesh.h"

#include <glad/glad.h>

#include <limits>
#include <stdexcept>

Mesh::Mesh(
    const float* vertices,
    std::size_t vertexByteSize,
    const unsigned int* indices,
    std::size_t indexCount
)
    : m_indexCount(indexCount)
{
    constexpr std::size_t floatsPerVertex = 8;
    constexpr std::size_t strideBytes = floatsPerVertex * sizeof(float);

    if (vertices == nullptr ||
        indices == nullptr ||
        vertexByteSize == 0 ||
        indexCount == 0 ||
        vertexByteSize % strideBytes != 0)
    {
        throw std::invalid_argument("Invalid mesh data");
    }
    
    if (indexCount >
        static_cast<std::size_t>(
            std::numeric_limits<GLsizei>::max()))
    {
        throw std::length_error("Too many mesh indices");
    }
    
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);
    
    glNamedBufferData(
        m_vbo,
        static_cast<GLsizei>(vertexByteSize),
        vertices,
        GL_STATIC_DRAW
    );
    
    glNamedBufferData(
        m_ebo,
        static_cast<GLsizeiptr>(indexCount * sizeof(unsigned int)),
        indices,
        GL_STATIC_DRAW
    );
    
    glVertexArrayVertexBuffer(
        m_vao,
        0,
        m_vbo,
        0,
        static_cast<GLsizei>(strideBytes)
    );
    
    // Position
    glVertexArrayElementBuffer(m_vao, m_ebo);
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);
    // Normal
    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, static_cast<GLuint>(3 * sizeof(float)));
    glVertexArrayAttribBinding(m_vao, 1, 0);
    // Texcoord
    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, static_cast<GLuint>(6 * sizeof(float)));
    glVertexArrayAttribBinding(m_vao, 2, 0);
}

Mesh::~Mesh()
{
    if (m_ebo != 0)
        glDeleteBuffers(1, &m_ebo);
    
    if (m_vbo != 0)
        glDeleteBuffers(1, &m_vbo);
    
    if (m_vao != 0)
        glDeleteVertexArrays(1, &m_vao);
}
    
void Mesh::Draw() const
{
    glBindVertexArray(m_vao);
    
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(m_indexCount),
        GL_UNSIGNED_INT,
        nullptr
    );
    
    glBindVertexArray(0);
}