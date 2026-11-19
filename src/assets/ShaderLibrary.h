#pragma once
#include <string>

class ShaderLibrary
{
public:
    static void Init(const char* path);
    static const std::string& GetSource() { return m_helperSource; }
    
    static std::string LoadShaderSource(const char* path);
    
private:
    static std::string m_helperSource;
};
