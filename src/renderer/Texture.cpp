#include "Texture.h"
#include "assets/ImageLoader.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <stdexcept>
#include <algorithm>
#include <cmath>

Texture::Texture(const std::string& path)
{
	ImageData image = ImageLoader::Load(path);

	if(image.data == nullptr)
		throw std::runtime_error("Image not found" + path);
	
	GLenum format = 0;
	GLenum internalFormat = 0;
	GLenum dataType = 0;

	switch(image.channels)
	{
		case 1:
			format = GL_RED;
			break;

		case 2:
			format = GL_RG;
			break;	
		
		case 3:
			format = GL_RGB;
			break;

		case 4:
			format = GL_RGBA;
			break;

		default:
			stbi_image_free(image.data);
			throw std::runtime_error("Unsupported channel count: " + path);
	}
	
	switch (image.bitsPerChannel)
	{
		case 8:
			dataType = GL_UNSIGNED_BYTE;
		
			switch (image.channels)
			{
				case 1:
					internalFormat = GL_R8;
					break;
				case 2:
					internalFormat = GL_RG8;
					break;
				case 3:
					internalFormat = GL_RGB8;
					break;
				case 4:
					internalFormat = GL_RGBA8;
					break;
			}
		break;
		
		case 16:
			dataType = GL_UNSIGNED_SHORT;
		
			switch (image.channels)
			{
				case 1:
					internalFormat = GL_R16;
					break;
				case 2:
					internalFormat = GL_RG16;
					break;
				case 3:
					internalFormat = GL_RGB16;
					break;
				case 4:
					internalFormat = GL_RGBA16;
					break;
			}
		break;
		
		default:
			stbi_image_free(image.data);
			throw std::runtime_error("Unsupported bit depth: " + path);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &m_id);

	glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	const int maxDimensions = std::max(image.width, image.height);
	const int mipLevels = 1 + static_cast<int>(std::floor(std::log2(maxDimensions)));
	
	GLint previousAlignment = 4;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	glTextureStorage2D(
		m_id,
		mipLevels,
		internalFormat,
		image.width,
		image.height
	);
	
	glTextureSubImage2D(
		m_id,
		0,
		0,
		0,
		image.width,
		image.height,
		format,
		dataType,
		image.data
	);

	glGenerateTextureMipmap(m_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
	stbi_image_free(image.data);
}

Texture::~Texture()
{
	if(m_id != 0)
	{
		glDeleteTextures(1, &m_id);
	}
}

void Texture::Bind(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0+slot);
	glBindTexture(GL_TEXTURE_2D, m_id);
}