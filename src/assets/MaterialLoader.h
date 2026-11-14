#pragma once
#include <filesystem>
#include <assimp/material.h>
#include <renderer/Texture.h>

#include "renderer/Material.h"

class MaterialLoader
{
public:
    static std::shared_ptr<Texture> LoadTexture(const std::string& path, ColorSpace cs);
    
    static void LoadSlot(
        const std::filesystem::path& modelDir,
        const aiMaterial* m,
        aiTextureType type,
        TextureSlot slot,
        ColorSpace cs,
        Material& out
    );
};
