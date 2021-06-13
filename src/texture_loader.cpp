#include "texture_loader.hpp"
#include "GL/gl3w.h"
#include "Tracy.hpp"
#include "stb_image.h"
#include <iostream>
#include <thread>

TextureLoader::~TextureLoader()
{
    for (ImageTexture& tex : textures) {
        unsigned int id = tex.id();
        glDeleteTextures(1, &id);
    }
}

ImageTexture* TextureLoader::get(TexID id)
{
    return &textures[id.id];
}

TexID TextureLoader::queueFile(std::string path)
{
    textures.push_back(ImageTexture(nullptr, 0, 0));
    TexID id { textures.size() - 1 };
    paths_queue.push_back(std::make_pair(id, path));
    return id;
}

TexID TextureLoader::addMem(unsigned char* data, unsigned int width, unsigned int height)
{
    textures.push_back(ImageTexture(data, width, height));
    return TexID { textures.size() - 1 };
}

struct ImageDesc {
    unsigned char* data;
    unsigned int width;
    unsigned int height;
    unsigned int id;
};

void loadThread(tracy::Lockable<std::mutex>& mtx, std::vector<std::pair<TexID, std::string>>& queue, std::vector<ImageDesc>& buffers, tracy::Lockable<std::mutex>& buf_mtx)
{
    ZoneScoped;

    while (true) {
        mtx.lock();
        LockMark(mtx);
        if (queue.empty()) {
            mtx.unlock();
            break;
        }

        auto p = queue.back();
        queue.pop_back();
        mtx.unlock();
        ZoneScoped;
        ZoneText(p.second.c_str(), p.second.size());

        int x, y, n;
        unsigned char* data = stbi_load(p.second.c_str(), &x, &y, &n, 4);

        if (data == NULL) {
            std::cerr << "Texture loading failed: " << stbi_failure_reason() << " (" << p.second << ")" << std::endl;
        }
        ImageDesc desc;
        desc.data = data;
        desc.id = p.first.id;
        desc.width = x;
        desc.height = y;

        buf_mtx.lock();
        LockMark(buf_mtx);
        buffers.push_back(desc);
        buf_mtx.unlock();
    }
}

void TextureLoader::load()
{
    ZoneScoped;
    constexpr unsigned int N = 4;
    std::array<std::thread, N> threads;
    TracyLockable(std::mutex, queue_mtx);
    TracyLockable(std::mutex, buf_mtx);

    std::vector<ImageDesc> buffers;
    buffers.reserve(paths_queue.size());

    for (unsigned int i = 0; i < N; i++) {
        threads[i] = std::thread(loadThread, std::ref(queue_mtx), std::ref(paths_queue), std::ref(buffers), std::ref(buf_mtx));
    }

    for (unsigned int i = 0; i < N; i++) {
        threads[i].join();
    }

    for (ImageDesc desc : buffers) {
        textures[desc.id] = ImageTexture(desc.data, desc.width, desc.height);
    }
}