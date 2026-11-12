#pragma once
#include <memory>
#include <string>

enum class GraphicsAPI
{
    OpenGL,
    Vulkan
};

class ShaderCompiler
{
public:
    static std::unique_ptr<IShader> Create(
        GraphicsAPI api,
        const std::string& filepath
    );
};
