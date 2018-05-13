#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "texture.hpp"
#include "GL/gl3w.h"
#include <iostream>

ImageTexture::ImageTexture(std::string path)
{
    int x, y, n;
    unsigned char* data = stbi_load(path.c_str(), &x, &y, &n, 4);

    if (data == NULL)
    {
        std::cerr << "Texture loading failed: " << stbi_failure_reason() << std::endl;
    }
    m_width = x;
    m_height = y;
    m_channels = n;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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
