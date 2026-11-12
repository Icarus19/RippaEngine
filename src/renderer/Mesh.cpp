#include "Mesh.h"

#include <glad/glad.h>
#include <utility>


Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)//, std::vector<Texture> textures)
    : m_vertices(std::move(vertices)),
      m_indices(std::move(indices)),
      //m_textures(std::move(textures)),
      m_indexCount(m_indices.size())
{
    SetupMesh();
}

Mesh Mesh::CreatePlane()
{
    std::vector<Vertex> vertices = {
        // Position                                     Normal                                      TexCoord
        // Front
        { {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f,  1.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f,  1.0f}, {0.0f, 1.0f} },
    };  
    std::vector<unsigned int> indices = {
        0, 1, 2,        2, 3, 0
    };
    
    return Mesh(std::move(vertices), std::move(indices));//, std::move(m_textures));
}

Mesh Mesh::CreateCube()
{
    std::vector<Vertex> vertices = {
        // Position                                     Normal                                      TexCoord
        // Front
        { {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f,  1.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f,  1.0f}, {0.0f, 1.0f} },
    };  
    std::vector<unsigned int> indices = {
        0, 1, 2,        2, 3, 0
    };
    
    return Mesh(std::move(vertices), std::move(indices));//, std::move(m_textures));
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

Mesh::Mesh(Mesh&& other) noexcept
    : m_vertices(std::move(other.m_vertices)),
    m_indices(std::move(other.m_indices)),
    m_vbo(std::move(other.m_vao)),
    m_ebo(std::move(other.m_vbo)),
    m_vao(std::move(other.m_ebo)),
    m_indexCount(std::move(other.m_indexCount))
{
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

void Mesh::BindRaytracingBuffers(
    GLuint vertexBinding,
    GLuint indexBinding
) const
{
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        vertexBinding,
        m_vbo
    );    
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        indexBinding,
        m_ebo
    );
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

void Mesh::SetupMesh()
{
    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glCreateBuffers(1, &m_ebo);
    
    glNamedBufferData(
        m_vbo,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data(),
        GL_STATIC_DRAW
    );
    
    glNamedBufferData(
        m_ebo,
        static_cast<GLsizeiptr>(m_indexCount * sizeof(unsigned int)),
        m_indices.data(),
        GL_STATIC_DRAW
    );
    
    constexpr GLsizei stride = sizeof(Vertex);
    
    glVertexArrayVertexBuffer(
        m_vao,
        0,
        m_vbo,
        0,
        stride
    );
    
    glVertexArrayElementBuffer(m_vao, m_ebo);
    
    // Position
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);
    // Normal
    glEnableVertexArrayAttrib(m_vao, 1);
    glVertexArrayAttribFormat(m_vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    glVertexArrayAttribBinding(m_vao, 1, 0);
    // TexCoord
    glEnableVertexArrayAttrib(m_vao, 2);
    glVertexArrayAttribFormat(m_vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
    glVertexArrayAttribBinding(m_vao, 2, 0);
}