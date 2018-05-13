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
#endif
