#include "ShaderCompiler.h"

#include <memory>

std::unique_ptr<IShader> ShaderCompiler::Create(
    GraphicsAPI api,
    const std::string& filepath)
{
    switch (api)
    {
        case GraphicsAPI::OPENGL:
            return std::make_unique<OpenGLShader>(filepath);
        
        case GraphicsAPI::VULKAN:
            return std::make_unique<VulkanShader>(filepath);
    }
    
    return nullptr;
}
