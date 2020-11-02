#ifndef TEXTURE_LOADER_HPP
#define TEXTURE_LOADER_HPP
#include <vector>
#include "texture.hpp"

struct TexID {
    unsigned int id;
};

class TextureLoader {
public:
    TextureLoader();
    ~TextureLoader();
    TexID add(ImageTexture tex);
    ImageTexture* get(TexID id);

    std::vector<ImageTexture> textures;
};
#endif