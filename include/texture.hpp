#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <string>
#include "GL/gl3w.h"

void set_tex_params();
unsigned int create_texture(unsigned int width, unsigned int height, int internal_format, int format);

class ImageTexture
{
public:
    ImageTexture() {};
    ImageTexture(std::string path);
    virtual ~ImageTexture();
    unsigned int id();
    unsigned int width();
    unsigned int height();

protected:
    unsigned int m_id, m_width, m_height, m_channels;
};

class HDRTexture: public ImageTexture
{
public:
    HDRTexture(std::string path);
};

class Cubemap
{
public:
    Cubemap(std::string up, std::string down, std::string left, std::string right, std::string front, std::string back);
    ~Cubemap();
    unsigned int id();

private:
    unsigned int m_id;
};
#endif
