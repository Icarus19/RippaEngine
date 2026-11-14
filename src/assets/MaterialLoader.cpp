#include "MaterialLoader.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <assimp/material.h>
#include <assimp/types.h>
#include <renderer/Texture.h>

#include "renderer/Material.h"

std::shared_ptr<Texture> MaterialLoader::LoadTexture(const std::string& path, ColorSpace cs)
{
    static std::unordered_map<std::string, std::weak_ptr<Texture>> cache;
    if (auto it = cache.find(path); it != cache.end())
        if (auto sp = it->second.lock()) return sp;

    auto tex = std::make_shared<Texture>(path, cs);
    cache[path] = tex;
    return tex;
}

void MaterialLoader::LoadSlot(const std::filesystem::path& modelDir, const aiMaterial* m,
              aiTextureType type, TextureSlot slot, ColorSpace cs, Material& out)
{
    if (m->GetTextureCount(type) == 0) return;

    aiString file;
    m->GetTexture(type, 0, &file);
    std::string full = (modelDir / file.C_Str()).string();
    std::replace(full.begin(), full.end(), '\\', '/');

    try {
        out.SetTexture(slot, LoadTexture(full, cs));
    } catch (const std::exception& e) {
        std::cerr << "Texture failed: " << full << " (" << e.what() << ")\n";
    }
}