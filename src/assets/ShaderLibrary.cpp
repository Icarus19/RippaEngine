#include "ShaderLibrary.h"

#include <fstream>       
#include <sstream>       
#include <iostream>      
#include <regex>         
#include <algorithm>     
#include <stdexcept>     
#include <utility>

ShaderSource ShaderLibrary::LoadShaderSource(
    const std::filesystem::path& path
)
{
    PreprocessContext context;
    context.rootDirectory = std::filesystem::absolute("shaders");
    ShaderSource result;
    
    result.source = PreProcessShader(path, context, true);
    result.sourceFiles = std::move(context.sourceFiles);
    
    return result;
}

int ShaderLibrary::GetSourceID(
    const std::filesystem::path& path,
    PreprocessContext& context
)
{
    auto canonical = std::filesystem::weakly_canonical(path);
    std::string key = canonical.string();
    
    auto it = context.sourceIDs.find(key);
    
    if (it != context.sourceIDs.end())
        return it->second;
    
    int id = static_cast<int>(context.sourceFiles.size());
    
    context.sourceIDs[key] = id;
    context.sourceFiles.push_back(canonical);
    
    return id;
}

std::string ShaderLibrary::PreProcessShader(
    const std::filesystem::path& path,
    PreprocessContext& context,
    bool isRoot
)
{
    auto absolutePath = std::filesystem::weakly_canonical(path);
    std::string key = absolutePath.string();
    
    std::stringstream output;
    std::string line;
    
    int lineNumber = 0;
    int sourceID = GetSourceID(absolutePath, context);
    
    if (!isRoot)
    {
        output << "#line 1 " << sourceID << '\n';
    }
    
    // Handle pragma
    if (context.pragmaOnceFiles.contains(key))
        return "";
    
    // Circular exceptions
    if (context.activeIncludes.contains(key))
    {
        throw std::runtime_error(
            "Circular shader include detected: " + key
        );
    }
    
    std::ifstream file(absolutePath);
    
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open shader: " + key  
        );
    }
    
    context.activeIncludes.insert(key);
    
    static const std::regex includeRegex(
        R"REGEX(^\s*#include\s+"([^"]+)"\s*$)REGEX"
    );   
    static const std::regex extensionRegex(
        R"REGEX(^\s*#extension\s+(.+)$)REGEX"
    );
    static const std::regex versionRegex(
        R"REGEX(^\s*#version\b.*$)REGEX"
    );
    static const std::regex pragmaOnceRegex(
        R"REGEX(^\s*#pragma\s+once\s*$)REGEX"
    );
    
    while (std::getline(file, line))
    {
        ++lineNumber;
        
        std::smatch match;
        
        if (std::regex_match(line, match, includeRegex))
        {
            auto includeName = match[1].str();

            std::filesystem::path includePath;

            if (!includeName.empty() && includeName[0] == '/')
            {
                includePath = context.rootDirectory / includeName.substr(1);
            }
            else
            {
                includePath = absolutePath.parent_path() / includeName;
            }
            
            output << PreProcessShader(
                includePath,
                context,
                false
            );
            
            output  << "\n#line "
                    << (lineNumber + 1)
                    << " "
                    << sourceID
                    << "\n";
            
            continue;
        }
        
        if (std::regex_match(line, pragmaOnceRegex))
        {
            context.pragmaOnceFiles.insert(key);
            continue;
        }
        
        if (std::regex_match(line, match, extensionRegex))
        {
            std::string extension = match[1].str();
            
            if (std::find(
                context.extensions.begin(),
                context.extensions.end(),
                extension
            ) == context.extensions.end())
            {
                context.extensions.push_back(extension);
            }
            
            continue;
        }
        
        if (!isRoot && std::regex_match(line, versionRegex))
        {
            throw std::runtime_error(
                "#version direction inside included file: " + key  
            );
        }
        
        output << line << '\n';
    }
    
    context.activeIncludes.erase(key);
    
    std::string result = output.str();
    
    if (isRoot)
    {
        std::stringstream finalSource;
        std::istringstream input(result);
        
        bool versionFound = false;
        
        while (std::getline(input, line))
        {
            finalSource << line << '\n';
            
            if (!versionFound && std::regex_match(line, versionRegex))
            {
                versionFound = true;
                
                for (const auto& extension : context.extensions)
                    finalSource << "#extension " << extension << '\n';
            }
        }
    
        if (!versionFound)
        {
            throw std::runtime_error(
                "Root shader is missing #version: " + key  
            );
        }
    
        result = finalSource.str();
    }
    
    return result;
}