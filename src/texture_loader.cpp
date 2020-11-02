#include "texture_loader.hpp"
#include <iostream>
#include "GL/gl3w.h"

TextureLoader::TextureLoader()
{
}

TextureLoader::~TextureLoader()
{
    for (ImageTexture& tex : textures)
    {
        unsigned int id = tex.id();
        glDeleteTextures(1, &id);
    }
}

TexID TextureLoader::add(ImageTexture tex)
{
    textures.push_back(std::move(tex));
    return {textures.size()-1};
}

ImageTexture* TextureLoader::get(TexID id)
{
    return &textures[id.id];
}
