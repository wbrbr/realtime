// don't define because assimp already has it
#define STB_IMAGE_IMPLEMENTATION
#include "texture.hpp"
#include "GL/gl3w.h"
#include "Tracy.hpp"
#include "TracyOpenGL.hpp"
#include "stb_image.h"
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
ImageTexture::ImageTexture(std::string path, bool srgb)
{
    ZoneScoped;
    ZoneText(path.c_str(), path.size());
    TracyGpuZone("ImageTexture::ImageTexture");

    int x, y, n;
    unsigned char* data = stbi_load(path.c_str(), &x, &y, &n, 4);

    if (data == NULL) {
        std::cerr << "Texture loading failed: " << stbi_failure_reason() << " (" << path << ")" << std::endl;
        exit(1);
    }
    m_width = x;
    m_height = y;
    m_channels = n;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

ImageTexture::ImageTexture(unsigned char* data, unsigned int width, unsigned int height)
{
    ZoneScoped;
    TracyGpuZone("ImageTexture::ImageTexture");
    m_width = width;
    m_height = height;
    m_channels = 4;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

ImageTexture::~ImageTexture()
{
}

unsigned int ImageTexture::id() const
{
    return m_id;
}

unsigned int ImageTexture::width() const
{
    return m_width;
}

unsigned int ImageTexture::height() const
{
    return m_height;
}

unsigned char* load(std::string path, int* x, int* y)
{
    unsigned char* data = stbi_load(path.c_str(), x, y, nullptr, 4);
    if (data == NULL) {
        std::cerr << "Failed to load " << path << ": " << stbi_failure_reason() << std::endl;
    }
    return data;
}

float* load_hdr(std::string path, int* w, int* h)
{
    float* data = stbi_loadf(path.c_str(), w, h, nullptr, 3);
    if (data == NULL) {
        std::cerr << "Failed to load " << path << ": " << stbi_failure_reason() << std::endl;
    }
    return data;
}

// CUBEMAP

Cubemap::Cubemap(std::string up, std::string down, std::string left, std::string right, std::string front, std::string back)
{
    int width, height;
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    float* data = load_hdr(up, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    data = load_hdr(down, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    data = load_hdr(left, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    data = load_hdr(right, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    data = load_hdr(front, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    data = load_hdr(back, &width, &height);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap::Cubemap(unsigned int face_width, unsigned int face_height)
{
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA16F, (int)face_width, (int)face_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA16F, (int)face_width, (int)face_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA16F, (int)face_width, (int)face_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA16F, (int)face_width, (int)face_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA16F, (int)face_width, (int)face_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA16F, (int)face_width, (int)face_height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap::~Cubemap()
{
    glDeleteTextures(1, &m_id);
}

unsigned int Cubemap::id() const
{
    return m_id;
}
