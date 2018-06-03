#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <string>

class ImageTexture
{
public:
    ImageTexture(std::string path);
    ~ImageTexture();
    unsigned int id();
    unsigned int width();
    unsigned int height();

private:
    unsigned int m_id, m_width, m_height, m_channels;
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
