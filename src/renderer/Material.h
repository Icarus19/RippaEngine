#pragma once
#include <array>
#include <memory>
#include <glm/glm.hpp>
#include <renderer/Texture.h>

enum class TextureSlot : unsigned char
{
    Albedo,
    Normal,
    Specular,
    Alpha,
    AO,
    Emissive,
    Count // Not a map, used for list size
};

class Material
{
public:
    glm::vec3 albedo   {1.0f};   // multiplied with the albedo texture, or used alone if none
    float     roughness = 0.5f;
    float     metallic  = 0.0f;
    glm::vec3 emission {0.0f};

    void SetTexture(TextureSlot slot, std::shared_ptr<Texture> tex)
    {
        m_textures[static_cast<size_t>(slot)] = std::move(tex);
    }

    const Texture* GetTexture(TextureSlot slot) const
    {
        return m_textures[static_cast<size_t>(slot)].get();
    }

    // Rasterizer path: texture unit == slot index
    void Bind() const
    {
        for (size_t i = 0; i < m_textures.size(); ++i)
            if (m_textures[i]) m_textures[i]->Bind(static_cast<unsigned>(i));
    }
    
private:
    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureSlot::Count)> m_textures;
};
