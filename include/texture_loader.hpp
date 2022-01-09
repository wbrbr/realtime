#ifndef TEXTURE_LOADER_HPP
#define TEXTURE_LOADER_HPP
#include "texture.hpp"
#include <utility>
#include <vector>
#include <glm/vec4.hpp>

struct TexID {
    size_t id;
};


class TextureLoader {
public:
    ~TextureLoader();
    TexID queueFile(std::string path, glm::vec4 defaultColor = glm::vec4(1,0,1,1));
    TexID addMem(unsigned char* data, unsigned int w, unsigned int h);
    ImageTexture* get(TexID id);
    void load();

    std::vector<ImageTexture> textures;

    struct QueuedTexture {
        TexID id;
        std::string path;
        glm::vec4 defaultColor;
    };

private:
    std::vector<QueuedTexture> paths_queue;
};
#endif
