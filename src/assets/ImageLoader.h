#pragma once

#include <string>

struct ImageData
{
    int width = 0;
    int height = 0;
    int channels = 0;
    
    unsigned char* data = nullptr;
};

class ImageLoader
{
public:
static ImageData Load(const std::string& path);    
};
