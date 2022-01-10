#include "texture_loader.hpp"
#include "GL/gl3w.h"
#include "Tracy.hpp"
#include "stb_image.h"
#include <iostream>
#include <thread>
#include <glm/gtc/type_ptr.hpp>
#include <mutex>

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

TexID TextureLoader::queueFile(std::string path, glm::vec4 defaultColor)
{
    textures.push_back(ImageTexture(nullptr, 0, 0));
    QueuedTexture queued_tex;
    queued_tex.id = TexID { textures.size() - 1 };
    queued_tex.path = path;
    queued_tex.defaultColor = defaultColor;
    paths_queue.push_back(queued_tex);
    return queued_tex.id;
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

void loadThread(std::mutex& mtx, std::vector<TextureLoader::QueuedTexture>& queue, std::vector<ImageDesc>& buffers, std::mutex& buf_mtx)
{
    ZoneScoped;

    while (true) {
        mtx.lock();
        if (queue.empty()) {
            mtx.unlock();
            break;
        }

        TextureLoader::QueuedTexture queued_tex = queue.back();
        queue.pop_back();
        mtx.unlock();
        ZoneScoped;
        ZoneText(queued_tex.path.c_str(), queued_tex.path.size());

        int x, y, n;
        unsigned char* data = stbi_load(queued_tex.path.c_str(), &x, &y, &n, 4);

        ImageDesc desc;
        if (data == NULL) {
            std::cerr << "Texture loading failed: " << stbi_failure_reason() << " (" << queued_tex.path << ")" << std::endl;
            desc.data = static_cast<unsigned char*>(malloc(4));
            desc.data[0] = (unsigned char)(queued_tex.defaultColor.r * 255.99f);
            desc.data[1] = (unsigned char)(queued_tex.defaultColor.g * 255.99f);
            desc.data[2] = (unsigned char)(queued_tex.defaultColor.b * 255.99f);
            desc.data[3] = (unsigned char)(queued_tex.defaultColor.a * 255.99f);
            desc.id = queued_tex.id.id;
            desc.width = 1;
            desc.height = 1;
        } else {
            desc.data = data;
            desc.id = queued_tex.id.id;
            desc.width = x;
            desc.height = y;
        }

        buf_mtx.lock();
        buffers.push_back(desc);
        buf_mtx.unlock();
    }
}

void TextureLoader::load()
{
    ZoneScoped;
    constexpr unsigned int N = 4;
    std::array<std::thread, N> threads;
    std::mutex queue_mtx;
    std::mutex buf_mtx;

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
