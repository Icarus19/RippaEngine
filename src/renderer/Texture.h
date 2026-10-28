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
    std::string type; // This is not used for now because we use glenum currently. Will replace with a struct on further versions
};
