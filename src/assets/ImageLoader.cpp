#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "ImageLoader.h"
#include <iostream>

ImageData ImageLoader::Load(const std::string& path)
{
    ImageData image;
    
    image.pixels = stbi_load(
        path.c_str(),    
        &image.width,
        &image.height,
        &image.channels,
        0
    );
    
    if (image.pixels == nullptr)
    {
        std::cerr << "Failed to load image from path: " << path << '\n';
        std::cerr << "Reason: " << stbi_failure_reason() << '\n';
    }
    
    return image;
}