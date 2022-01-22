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
    auto it = m_paths_ids_map.find(path);
    if (it == m_paths_ids_map.end()) {
        textures.push_back(ImageTexture());
        QueuedTexture queued_tex;
        queued_tex.id = TexID { textures.size() - 1 };
        queued_tex.path = path;
        queued_tex.defaultColor = defaultColor;
        paths_queue.push_back(queued_tex);
        m_paths_ids_map[path] = queued_tex.id;
        return queued_tex.id;
    } else {
        return it->second;
    }
}

TexID TextureLoader::addMem(unsigned char* data, unsigned int width, unsigned int height, unsigned int type)
{
    textures.push_back(ImageTexture(data, width, height, type));
    return TexID { textures.size() - 1 };
}

struct ImageDesc {
    unsigned char* data;
    unsigned int width;
    unsigned int height;
    unsigned int id;
    unsigned int type;
};

struct CompressedImageDesc {
    unsigned char* data;
    unsigned int width;
    unsigned int height;
    unsigned int id;
    unsigned int type;
    unsigned int size;
};

// TODO: put this in util.cpp
static bool endsWith(const char* str, const char* substr)
{
    size_t str_len = strlen(str);
    size_t substr_len = strlen(substr);

    if (substr_len > str_len) {
        return false;
    }

    for (size_t i = 0; i < substr_len; i++) {
        size_t str_idx = str_len - 1 - i;
        size_t substr_idx = substr_len - 1 - i;

        if (str[str_idx] != substr[substr_idx]) {
            return false;
        }
    }

    return true;
}

// TODO: don't use the same mutex for the two vectors
void loadThread(std::mutex& queue_mtx, std::vector<TextureLoader::QueuedTexture>& queue, std::vector<ImageDesc>& buffers, std::vector<CompressedImageDesc>& compressed_buffers, std::mutex& buf_mtx)
{
    ZoneScoped;

    while (true) {
        queue_mtx.lock();
        if (queue.empty()) {
            queue_mtx.unlock();
            break;
        }

        TextureLoader::QueuedTexture queued_tex = queue.back();
        queue.pop_back();
        queue_mtx.unlock();
        ZoneScoped;
        ZoneText(queued_tex.path.c_str(), queued_tex.path.size());

        std::cout << "Loading " << queued_tex.path << std::endl;

        bool ok = true;
        if (endsWith(queued_tex.path.c_str(), ".dds")) {
            FILE* fp = fopen(queued_tex.path.c_str(), "rb");
            uint8_t header[128];
            fread(header, 1, 128, fp);
            if (header[0] != 'D' || header[1] != 'D' || header[2] != 'S' || header[3] != ' ') {
                std::cerr << "DDS file doesn't have the magic number: " << queued_tex.path << std::endl;
            }
            
            uint32_t h = header[12] | header[13] << 8 | header[14] << 16 | header[15] << 24;
            uint32_t w = header[16] | header[17] << 8 | header[18] << 16 | header[19] << 24;
            char fourCC[5];
            memcpy(fourCC, &header[84], 4);
            fourCC[4] = 0;
            //std::cerr << fourCC << ": " << w << "x" << h << std::endl;

            fseek(fp, 0, SEEK_END);
            size_t file_size = ftell(fp);
            fseek(fp, 128, SEEK_SET);

            unsigned char* data = static_cast<unsigned char*>(malloc(file_size-128));
            if (fread(data, 1, file_size - 128, fp) != file_size - 128) {
                std::string err_msg = "Failed to read " + queued_tex.path;
                perror(err_msg.c_str());
            }

            unsigned int type;
            unsigned int block_width, block_height;
            unsigned int bytes_per_block; 
            fclose(fp);
            if (strcmp(fourCC, "DXT1") == 0) {
                type = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                block_width = block_height = 4;
                bytes_per_block = 8;
            } else if (strcmp(fourCC, "DXT3") == 0) {
                type = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; 
                block_width = block_height = 4;
                bytes_per_block = 16;
            } else if (strcmp(fourCC, "DXT5") == 0) {
                type = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                block_width = block_height = 4;
                bytes_per_block = 16;
            } else {
                std::cerr << "Unknown DDS format: " << fourCC << std::endl;
                ok = false;
            }

            if (ok) {
                CompressedImageDesc desc;
                desc.data = data;
                desc.id = queued_tex.id.id;
                desc.width = w;
                desc.height = h;
                desc.type = type;
                desc.size = bytes_per_block * w * h / (block_width * block_height);
                assert(desc.size < file_size - 128);
                buf_mtx.lock();
                compressed_buffers.push_back(desc);
                buf_mtx.unlock();
            }
        } else {
            int x, y, n;
            unsigned char* data = stbi_load(queued_tex.path.c_str(), &x, &y, &n, 4);
            ImageDesc desc;
            if (data) {
                desc.data = data;
                desc.id = queued_tex.id.id;
                desc.width = x;
                desc.height = y;
                desc.type = GL_RGBA;
                buf_mtx.lock();
                buffers.push_back(desc);
                buf_mtx.unlock();
            } else {
                ok = false; 
            }
        }

        if (!ok) {
            ImageDesc desc;
            std::cerr << "Texture loading failed:" << queued_tex.path << std::endl;
            desc.data = static_cast<unsigned char*>(malloc(4));
            desc.data[0] = (unsigned char)(queued_tex.defaultColor.r * 255.99f);
            desc.data[1] = (unsigned char)(queued_tex.defaultColor.g * 255.99f);
            desc.data[2] = (unsigned char)(queued_tex.defaultColor.b * 255.99f);
            desc.data[3] = (unsigned char)(queued_tex.defaultColor.a * 255.99f);
            desc.id = queued_tex.id.id;
            desc.width = 1;
            desc.height = 1;
            desc.type = GL_RGBA;

            buf_mtx.lock();
            buffers.push_back(desc);
            buf_mtx.unlock();
        }
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
    std::vector<CompressedImageDesc> compressed_buffers;
    buffers.reserve(paths_queue.size());

    for (unsigned int i = 0; i < N; i++) {
        threads[i] = std::thread(loadThread, std::ref(queue_mtx), std::ref(paths_queue), std::ref(buffers), std::ref(compressed_buffers), std::ref(buf_mtx));
    }

    for (unsigned int i = 0; i < N; i++) {
        threads[i].join();
    }

    for (ImageDesc desc : buffers) {
        textures[desc.id] = ImageTexture(desc.data, desc.width, desc.height, desc.type);
        free(desc.data);
    }

    for (CompressedImageDesc desc : compressed_buffers) {
        textures[desc.id] = ImageTexture(desc.data, desc.width, desc.height, desc.type, desc.size);
        free(desc.data);
    }

    buffers.clear();
}
