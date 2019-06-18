#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.hpp"
#include <iostream>

void set_tex_params()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

unsigned int create_texture(unsigned int width, unsigned int height, int internal_format, int format)
{
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, GL_FLOAT, NULL);
    set_tex_params();
    return id;
}
// IMAGE TEXTURE
ImageTexture::ImageTexture(std::string path)
{
    int x, y, n;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path.c_str(), &x, &y, &n, 3);

    if (data == NULL)
    {
        std::cerr << "Texture loading failed: " << stbi_failure_reason() << " (" << path << ")" << std::endl;
    }
    m_width = x;
    m_height = y;
    m_channels = n;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}

ImageTexture::~ImageTexture()
{
    glDeleteTextures(1, &m_id);
}

unsigned int ImageTexture::id()
{
    return m_id;
}

unsigned int ImageTexture::width()
{
    return m_width;
}

unsigned int ImageTexture::height()
{
    return m_height;
}

unsigned char* load(std::string path, int* x, int* y)
{
    unsigned char* data = stbi_load(path.c_str(), x, y, nullptr, 4);
    if (data == NULL)
    {
        std::cerr << "Texture loading failed: " << stbi_failure_reason() << std::endl;
    }
    return data;
}

// CUBEMAP

Cubemap::Cubemap(std::string up, std::string down, std::string left, std::string right, std::string front, std::string back)
{
    int width, height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    unsigned char* data = load(up, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    data = load(down, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    data = load(left, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    data = load(right, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    data = load(front, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    data = load(back, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

Cubemap::~Cubemap()
{
    glDeleteTextures(1, &m_id);
}

unsigned int Cubemap::id()
{
    return m_id;
}

HDRTexture::HDRTexture(std::string path)
{
    int x, y, n;
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(path.c_str(), &x, &y, &n, 3);

    if (data == NULL)
    {
        std::cerr << "Texture loading failed: " << stbi_failure_reason() << " (" << path << ")" << std::endl;
    }
    m_width = x;
    m_height = y;
    m_channels = n;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}
