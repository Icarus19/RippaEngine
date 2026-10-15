#pragma once

#include <string>

class Texture
{
public:
    explicit Texture(const std::string &path = "./assets/default.png");
    ~Texture();
    
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    void Bind(unsigned int slot = 0) const;
    
private:
    unsigned int m_id = 0;
};
