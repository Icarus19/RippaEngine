#pragma once

#include <string>

enum class ColorSpace
{
    Linear,
    SRGB
};

class Texture
{
public:
    explicit Texture(
        const std::string &path = "./assets/default.png",
        ColorSpace cs = ColorSpace::Linear
    );
    ~Texture();
    
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    
    void Bind(unsigned int slot = 0) const;
    
    unsigned int GetID() const { return m_id; }
    
private:
    unsigned int m_id = 0;
    std::string m_path;
};
