#pragma once

#include "renderer/Shader.h"

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>


class ShaderLibrary
{
public:
    static ShaderSource LoadShaderSource(const std::filesystem::path& path);

private:
    struct PreprocessContext
    {
        std::filesystem::path rootDirectory;
        
        std::unordered_set<std::string> activeIncludes;
        std::unordered_set<std::string> pragmaOnceFiles;
        
        std::unordered_map<std::string, int> sourceIDs;
        
        std::vector<std::filesystem::path> sourceFiles;
        std::vector<std::string> extensions;
    };
    
    static int GetSourceID(
        const std::filesystem::path& path,
        PreprocessContext& context
    );
    static std::string PreProcessShader(
        const std::filesystem::path& path,
        PreprocessContext& context,
        bool isRoot
    );
};
