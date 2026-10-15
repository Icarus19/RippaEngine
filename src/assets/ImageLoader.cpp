#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "ImageLoader.h"
#include <iostream>

ImageData ImageLoader::Load(const std::string& path)
{
    ImageData image;
    
    stbi_set_flip_vertically_on_load(true); // Please, why?!
    
    if (stbi_is_16_bit(path.c_str()))
    {
        image.data = stbi_load_16(
            path.c_str(),
            &image.width,
            &image.height,
            &image.sourceChannels,
            STBI_rgb_alpha
            );
        image.channels = 4;
        image.bitsPerChannel = 16;
        std::cout
    << "Image: " << path
    << "\nDimensions: " << image.width
    << " x " << image.height
    << "\nChannels returned: " << image.channels
    << "\nBits per channel: " << image.bitsPerChannel
    << '\n';
    } else {
        image.data = stbi_load(
            path.c_str(),    
            &image.width,
            &image.height,
            &image.sourceChannels,
            STBI_rgb_alpha
        );
        image.channels = 4;
        image.bitsPerChannel = 8;
        std::cout
        << "Image: " << path
        << "\nDimensions: " << image.width
        << " x " << image.height
        << "\nChannels returned: " << image.channels
        << "\nBits per channel: " << image.bitsPerChannel
        << '\n';
    }
    
    if (image.data == nullptr)
    {
        std::cerr << "Failed to load image from path: " << path << '\n';
        std::cerr << "Reason: " << stbi_failure_reason() << '\n';
    }
    
    return image;
}