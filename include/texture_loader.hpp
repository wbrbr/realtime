#ifndef TEXTURE_LOADER_HPP
#define TEXTURE_LOADER_HPP
#include <vector>
#include <utility>
#include "texture.hpp"

struct TexID {
    unsigned int id;
};

class TextureLoader {
public:
    ~TextureLoader();
    TexID queueFile(std::string path);
    TexID addMem(unsigned char* data, unsigned int w, unsigned int h);
    ImageTexture* get(TexID id);
    void load();

    std::vector<ImageTexture> textures;

private:
    std::vector<std::pair<TexID, std::string>> paths_queue;
};
#endif