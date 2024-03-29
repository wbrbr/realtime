#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include "Tracy.hpp"
#include "TracyOpenGL.hpp"
#include "assimp/Importer.hpp"
#include "assimp/pbrmaterial.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "camera.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "light.hpp"
#include "mesh.hpp"
#include "object.hpp"
#include "renderer.hpp"
#include "shader.hpp"
#include "stb_image.h"
#include "texture.hpp"
#include "texture_loader.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <optional>

#define DBG_MODE 1

const unsigned int WIDTH = 1600;
const unsigned int HEIGHT = 900;

bool flyMode = true;

struct Context {
    TextureLoader loader;
    Renderer renderer;
    Scene scene;
    Camera camera;

    Context()
        : renderer(WIDTH, HEIGHT, loader)
    {}
};


TexID loadTextureFromPath(aiString prefix, aiString fileName, const aiScene* scene, TextureLoader& loader, glm::vec4 defaultColor)
{
    ZoneScoped;
    // embedded texture
    if (fileName.C_Str()[0] == '*') {
        const aiTexture* embeddedTex = scene->GetEmbeddedTexture(fileName.C_Str());
        assert(embeddedTex->mHeight == 0); // must be a compressed texture
        int w, h, n;
        unsigned char* data = stbi_load_from_memory((unsigned char*)embeddedTex->pcData, embeddedTex->mWidth, &w, &h, &n, 4);
        if (data == nullptr) {
            std::cerr << "Embedded texture loading failed: " << stbi_failure_reason() << std::endl;
        }
        return loader.addMem(data, w, h, GL_RGBA);
    } else {
        aiString texPath = prefix;
        texPath.Append(fileName.C_Str());
        std::string p { texPath.C_Str() };
        std::replace(p.begin(), p.end(), '\\', '/');
        return loader.queueFile(p, defaultColor);
    }
}

void compute_aabb(Object& obj, const aiVector3D* vertices, size_t num_vertices)
{
    glm::vec3 min_ = glm::vec3(INFINITY, INFINITY, INFINITY);
    glm::vec3 max_ =  glm::vec3(-INFINITY, -INFINITY, -INFINITY);

    for (size_t i = 0; i < num_vertices; i++) {
        min_.x = fmin(min_.x, vertices[i].x);
        min_.y = fmin(min_.y, vertices[i].y);
        min_.z = fmin(min_.z, vertices[i].z);

        max_.x = fmax(max_.x, vertices[i].x);
        max_.y = fmax(max_.y, vertices[i].y);
        max_.z = fmax(max_.z, vertices[i].z);
    }

    obj.aabb.points[0] = glm::vec3(min_.x, min_.y, min_.z);
    obj.aabb.points[1] = glm::vec3(max_.x, min_.y, min_.z);
    obj.aabb.points[2] = glm::vec3(min_.x, max_.y, min_.z);
    obj.aabb.points[3] = glm::vec3(min_.x, min_.y, max_.z);
    obj.aabb.points[4] = glm::vec3(max_.x, min_.y, max_.z);
    obj.aabb.points[5] = glm::vec3(min_.x, max_.y, max_.z);
    obj.aabb.points[6] = glm::vec3(max_.x, max_.y, min_.z);
    obj.aabb.points[7] = glm::vec3(max_.x, max_.y, max_.z);
}


Object loadMesh(std::string path, const aiScene* scene, unsigned int mesh_index, TextureLoader& loader)
{
    ZoneScoped;

    aiMesh* m = scene->mMeshes[mesh_index];

    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 14 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m->mNumVertices * 3 * sizeof(float), m->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mNormals);
    if (m->HasTextureCoords(0)) {
        float* texcoords = static_cast<float*>(malloc(m->mNumVertices * 2 * sizeof(float)));
        for (unsigned int i = 0; i < m->mNumVertices; i++) {
            auto vec = m->mTextureCoords[0][i];
            texcoords[2 * i] = vec.x;
            texcoords[2 * i + 1] = vec.y;
        }
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 6 * sizeof(float), m->mNumVertices * 2 * sizeof(float), texcoords);
        free(texcoords);
    } else {
        float* zero = static_cast<float*>(malloc(m->mNumVertices * 2 * sizeof(float)));
        for (unsigned int i = 0; i < m->mNumVertices * 2; i++) {
            zero[i] = 0.f;
        }
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 6 * sizeof(float), m->mNumVertices * 2 * sizeof(float), zero);
        free(zero);
    }
    if (m->HasTangentsAndBitangents()) {
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 8 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mTangents);
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 11 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mBitangents);
    } else {
        std::vector<glm::vec3> tangents;
        std::vector<glm::vec3> bitangents;
        for (unsigned int i = 0; i < m->mNumVertices; i++) {
            glm::vec3 n(m->mNormals[i].x, m->mNormals[i].y, m->mNormals[i].z);
            glm::vec3 random_vector = glm::vec3((float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX);
            glm::vec3 tangent = glm::normalize(random_vector - n * dot(n, random_vector));
            glm::vec3 bitangent = glm::cross(n, tangent);
            tangents.push_back(tangent);
            bitangents.push_back(bitangent);
        }
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 8 * sizeof(float), m->mNumVertices * 3 * sizeof(float), tangents.data());
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 11 * sizeof(float), m->mNumVertices * 3 * sizeof(float), bitangents.data());
    }

    glCreateBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    unsigned int* indices = static_cast<unsigned int*>(malloc(m->mNumFaces * 3 * sizeof(unsigned int)));
    for (unsigned int i = 0; i < m->mNumFaces; i++) {
        aiFace f = m->mFaces[i];
        indices[3 * i] = f.mIndices[0];
        indices[3 * i + 1] = f.mIndices[1];
        indices[3 * i + 2] = f.mIndices[2];
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->mNumFaces * 3 * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    free(indices);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 6 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 8 * sizeof(float)));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 11 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    Mesh mesh;
    mesh.vao = vao;
    mesh.vbo = vbo;
    mesh.numVertices = m->mNumVertices;
    mesh.numIndices = 3 * m->mNumFaces;

    Object obj;
    obj.mesh = mesh;

    std::string prefixPath = std::filesystem::path(path).parent_path().string();
    aiString prefix(prefixPath.c_str());
    prefix.Append("/");
    aiString filePath;
    if (scene->HasMaterials()) {
        aiMaterial* material = scene->mMaterials[m->mMaterialIndex];
        material->GetTexture(aiTextureType_DIFFUSE, 0, &filePath);
        if (filePath.length > 0) {
            obj.material.albedoMap = loadTextureFromPath(prefix, filePath, scene, loader, glm::vec4(.7, .7, .7, 1));
        } else {
            std::cerr << "no baseColor texture" << std::endl;
            unsigned char color[] = { 255, 0, 255, 255 };
            obj.material.albedoMap = loader.addMem(color, 1, 1, GL_RGBA);
        }
        filePath.Clear();
        material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &filePath);
        if (filePath.length > 0) {
            obj.material.roughnessMetallicMap = loadTextureFromPath(prefix, filePath, scene, loader, glm::vec4(0,1,0,1));
        } else {
            std::cerr << "no roughness/metallic texture" << std::endl;
            // roughness = 1, metallic = 0
            unsigned char color[] = { 0, 255, 0, 255 };
            obj.material.roughnessMetallicMap = loader.addMem(color, 1, 1, GL_RGBA);
        }
        bool normalMap = true;
        if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
            material->GetTexture(aiTextureType_NORMALS, 0, &filePath);
            if (filePath.length == 0) {
                normalMap = false;
            } else {
                obj.material.normalMap = loadTextureFromPath(prefix, filePath, scene, loader, glm::vec4(0.5, 0.5, 1, 1));
            }
        } else {
            normalMap = false;
        }
        if (!normalMap) {
            std::cerr << "no normal map" << std::endl;
            unsigned char color[] = { 128, 128, 255, 255 };
            obj.material.normalMap = loader.addMem(color, 1, 1, GL_RGBA);
        }
    }

    compute_aabb(obj, m->mVertices, m->mNumVertices);
    return obj;
}



Scene loadFile(std::string path, TextureLoader& loader)
{
    ZoneScoped;
    Scene scene;

    Assimp::Importer importer;
    const aiScene* assimp_scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
    if (!assimp_scene) {
        std::cerr << importer.GetErrorString() << std::endl;
        return scene;
    }

    glm::vec3 scene_min = glm::vec3(INFINITY, INFINITY, INFINITY);
    glm::vec3 scene_max = glm::vec3(-INFINITY, -INFINITY, -INFINITY);

    scene.objects.resize(assimp_scene->mNumMeshes);

    for (unsigned int i = 0; i < assimp_scene->mNumMeshes; i++) {
        scene.objects[i] = loadMesh(path, assimp_scene, i, loader);

        glm::vec3 aabb_min = scene.objects[i].aabb.points[0];
        glm::vec3 aabb_max = scene.objects[i].aabb.points[7];

        scene_min.x = fmin(scene_min.x, aabb_min.x);
        scene_min.y = fmin(scene_min.y, aabb_min.y);
        scene_min.z = fmin(scene_min.z, aabb_min.z);
        scene_max.x = fmax(scene_max.x, aabb_max.x);
        scene_max.y = fmax(scene_max.y, aabb_max.y);
        scene_max.z = fmax(scene_max.z, aabb_max.z);
    }

    glm::vec3 extents = scene_max - scene_min;
    scene.radius = sqrt(2) * fmax(fmax(extents.x, extents.y), extents.z);

    return scene;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) {
        switch (key) {
			case GLFW_KEY_F: 
				flyMode = !flyMode;
				if (flyMode) {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				} else {
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				break;

			case GLFW_KEY_R:
            {
				Context* ctx = static_cast<Context*>(glfwGetWindowUserPointer(window));
				ctx->renderer.reloadShaders();
				break;
			}
		}
    }
}

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

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    Context* ctx = static_cast<Context*>(glfwGetWindowUserPointer(window));
    for (int i = 0; i < count; i++)
    {
        const char* path = paths[i];
        if (endsWith(path, ".gltf") || endsWith(path, ".fbx")) {
            std::cout << "Loading scene\n" << std::endl;
            ctx->scene = loadFile(std::string(path), ctx->loader);
            std::cout << "Loading textures\n" << std::endl;
            ctx->loader.load();
            std::cout << "Done.\n" << std::endl;
            ctx->camera.zFar = ctx->scene.radius;
            ctx->camera.zNear = ctx->camera.zFar / 1000;
        } else if (endsWith(path, ".hdr")) {
            ImageTexture tex(path);
            ctx->renderer.setSkyboxFromEquirectangular(tex, 512, 512);
        }
    }
}

GLFWwindow* initWindow()
{
    if (!glfwInit()) {
        return nullptr;
    }
    glfwWindowHint(GLFW_RESIZABLE, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Real-time rendering", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, key_callback);
    glfwSetDropCallback(window, drop_callback);
    glfwMakeContextCurrent(window);

    if (gl3wInit()) {
        glfwTerminate();
        return nullptr;
    }
    if (!gl3wIsSupported(4, 4)) {
        std::cout << "OpenGL 4.4 not supported" << std::endl;
        return nullptr;
    }

    return window;
}

void dbgcallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* data)
{
    if (DBG_MODE && type == GL_DEBUG_TYPE_ERROR) {
        std::cerr << "debug call: " << msg << std::endl;
    }
}


int main(int argc, char** argv)
{
    GLFWwindow* window = initWindow();
    if (!window) {
        return 1;
    }

    TracyGpuContext

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGuiIO& imio = ImGui::GetIO();

#if DBG_MODE
    glDebugMessageCallback(dbgcallback, NULL);
#endif
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    std::filesystem::current_path(BASE_DIR);


    Context ctx;
    glfwSetWindowUserPointer(window, &ctx);

    ctx.camera.setPosition(glm::vec3(0.f, 0.f, 3.f));
    //ImageTexture envmap("res/photo_studio_loft_hall_4k.hdr");

    //renderer.setSkyboxFromEquirectangular(envmap, 512, 512);
    //std::vector<Object> objects = loadFile(argv[1], loader);
    //loader.load();

    double lastCursorX, lastCursorY;
    float polar = 0.f; // [-pi/2, pi/2]
    float azim = 0.f; // [0, 2pi]
    glfwGetCursorPos(window, &lastCursorX, &lastCursorY);

    float walkSpeed = .8f;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        double currentX, currentY;
        glfwGetCursorPos(window, &currentX, &currentY);
        float xdelta = currentX - lastCursorX;
        float ydelta = currentY - lastCursorY;
        lastCursorX = currentX;
        lastCursorY = currentY;

        float sensitivity = 0.001;
        if (flyMode) {
            azim += -xdelta * sensitivity;
            polar += -ydelta * sensitivity;
        }
        glm::vec3 dir(0.f, 0.f, -1.f);
        dir = glm::rotateX(dir, polar);
        dir = glm::rotateY(dir, azim);
        dir = glm::normalize(dir);

        glm::vec3 up(0.f, 1.f, 0.f);
        glm::vec3 right = glm::cross(dir, up);
        ImGui::Begin("Renderer");
        ImGui::SliderFloat("Walk speed", &walkSpeed, 0.01f, 0.8f);
        const float speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 3.f * walkSpeed : walkSpeed;
        if (flyMode && !imio.WantCaptureKeyboard) {
            if (glfwGetKey(window, GLFW_KEY_A)) {
                ctx.camera.setPosition(ctx.camera.getPosition() - speed * right);
            }
            if (glfwGetKey(window, GLFW_KEY_D)) {
                ctx.camera.setPosition(ctx.camera.getPosition() + speed * right);
            }
            if (glfwGetKey(window, GLFW_KEY_W)) {
                ctx.camera.setPosition(ctx.camera.getPosition() + speed * dir);
            }
            if (glfwGetKey(window, GLFW_KEY_S)) {
                ctx.camera.setPosition(ctx.camera.getPosition() - speed * dir);
            }
            if (glfwGetKey(window, GLFW_KEY_SPACE)) {
                ctx.camera.setPosition(ctx.camera.getPosition() + speed * up);
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
                ctx.camera.setPosition(ctx.camera.getPosition() - speed * up);
            }
        }

        ctx.camera.setTarget(ctx.camera.getPosition() + dir);

        ctx.renderer.render(ctx.scene, ctx.camera);
        ImGui::Text("FPS: %.1f", imio.Framerate);

        ImGui::End();

        //ImGui::ShowDemoWindow();
        {
            ZoneScopedN("ImGui render");
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        {
            ZoneScopedN("Buffer swap");
            glfwSwapBuffers(window);
        }
        FrameMark;
        TracyGpuCollect;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
