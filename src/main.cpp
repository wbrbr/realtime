#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include <optional>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "shader.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "object.hpp"
#include "light.hpp"
#include "texture.hpp"
#include "renderer.hpp"

#define DBG_MODE 1

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    float r = glm::length(camera->getPosition());
    float r2 = r - 0.08f * yoffset;
    camera->setPosition(r2 / r * camera->getPosition());
}

GLFWwindow* initWindow()
{
    if (!glfwInit())
    {
        return nullptr;
    }
    glfwWindowHint(GLFW_RESIZABLE, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, true );

    GLFWwindow* window = glfwCreateWindow(800, 450, "Real-time rendering", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return nullptr;
    }
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);

    if (gl3wInit())
    {
        glfwTerminate();
        return nullptr;
    }
    if (!gl3wIsSupported(3, 3))
    {
        std::cout << "OpenGL 3.3 not supported" << std::endl;
        return nullptr;
    }
    
    return window;
}

void dbgcallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *msg, const void *data)
{
    if (DBG_MODE) {
        std::cout << "debug call: " << msg << std::endl;
    }
}

std::optional<Object> loadMesh(std::string path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
    if (!scene) {
        std::cerr << importer.GetErrorString() << std::endl;
        return {};
    }
    aiMesh* m = scene->mMeshes[0];

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
        for (unsigned int i = 0; i < m->mNumVertices; i++)
        {
            auto vec = m->mTextureCoords[0][i];
            texcoords[2*i] = vec.x;
            texcoords[2*i+1] = vec.y;
        }
        glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 6 * sizeof(float), m->mNumVertices * 2 * sizeof(float), texcoords);
        free(texcoords);
    } else {
        float* zero = static_cast<float*>(malloc(m->mNumVertices * 2 * sizeof(float)));
        for (unsigned int i = 0; i < m->mNumVertices * 2; i++)
        {
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
        for (unsigned int i = 0; i < m->mNumVertices; i++)
        {
            glm::vec3 n(m->mNormals[i].x, m->mNormals[i].y, m->mNormals[i].z);
            glm::vec3 random_vector = glm::vec3((float)rand()/(float)RAND_MAX, (float)rand()/(float)RAND_MAX, (float)rand()/(float)RAND_MAX);
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
        indices[3*i] = f.mIndices[0];
        indices[3*i+1] = f.mIndices[1];
        indices[3*i+2] = f.mIndices[2];
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m->mNumFaces*3*sizeof(unsigned int), indices, GL_STATIC_DRAW);
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
    mesh.numIndices = 3*m->mNumFaces;

    Object obj;
    obj.mesh = mesh;
    if (scene->HasMaterials()) {
        aiMaterial* material = scene->mMaterials[0];
        aiString prefix("../res/");
        aiString texPath;
        material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &texPath);
        if (texPath.length > 0) {
            prefix.Append(texPath.C_Str());
            ImageTexture* tex = new ImageTexture(prefix.C_Str());
            obj.material.albedoMap = tex;
        }
        material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &texPath);
        if (texPath.length > 0) {
            prefix.Set("../res/");
            prefix.Append(texPath.C_Str());
            ImageTexture* tex = new ImageTexture(prefix.C_Str());
            obj.material.roughnessMetallicMap = tex;
        }
        if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
            material->GetTexture(aiTextureType_NORMALS, 0, &texPath);
            prefix.Set("../res/");
            std::cout << texPath.C_Str() << std::endl;
            prefix.Append(texPath.C_Str());
            ImageTexture* tex = new ImageTexture(prefix.C_Str());
            obj.material.normalMap = tex;
        } else {
            std::cout << "no normal map" << std::endl;
            unsigned char color[] = { 128, 128, 255, 255 };
            ImageTexture* tex = new ImageTexture(color, 1, 1);
            obj.material.normalMap = tex;
        }
    }
    return obj;
}

int main()
{
    GLFWwindow* window = initWindow();
    if (!window)
    {
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGuiIO& imio = ImGui::GetIO();

    // glDebugMessageCallback(dbgcallback, NULL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Object suzanne = loadMesh("../meshes/Suzanne.gltf").value();
    // Object avocado = loadMesh("../res/Avocado.gltf").value();
    // Object suzanne = loadMesh("../res/suzanne2.obj").value();
    // suzanne.mesh = loadMesh("../meshes/suzanne2.obj").value();
    // suzanne.mesh = loadMesh("../meshes/Box.gltf").value();

    // Object plane = loadMesh("../meshes/plane.obj").value();
    // plane.mesh = loadMesh("../meshes/plane.obj").value();
    // plane.transform.position.y -= 0.5f;

    // Object cube = loadMesh("../meshes/cube.obj").value();
    // cube.mesh = loadMesh("../meshes/cube.obj").value();
    // cube.transform.scale = glm::vec3(.3f, .3f, .3f);

    /* Object cube;
    cube.mesh = loadMesh("../meshes/skybox.obj").value(); */

    /* glEnable(GL_TEXTURE_CUBE_MAP); */
    /* glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); */

    Camera camera;
    camera.setPosition(glm::vec3(0.f, 0.f, 3.f));
    glfwSetWindowUserPointer(window, &camera);

    ImageTexture albedotex1("../res/rustediron2_basecolor.png");
    ImageTexture metallictex1("../res/rustediron2_metallic.png");
    ImageTexture roughnesstex1("../res/rustediron2_roughness.png");
    ImageTexture normaltex1("../res/rustediron2_normal.png");
    ImageTexture albedotex2("../res/metalgrid3_basecolor.png");
    ImageTexture metallictex2("../res/metalgrid3_metallic.png");
    ImageTexture roughnesstex2("../res/metalgrid3_roughness.png");
    ImageTexture normaltex2("../res/metalgrid3_normal-ogl.png");

	// suzanne.material.albedoMap = &albedotex1;
	// suzanne.material.metallicMap = &metallictex1;
	// suzanne.material.roughnessMap = &roughnesstex1;
	// suzanne.material.normalMap = &normaltex1;
	// plane.material.albedoMap = &albedotex2;
	// plane.material.metallicMap = &metallictex2;
	// plane.material.roughnessMap = &roughnesstex2;
	// plane.material.normalMap = &normaltex2;
    // cube.material.albedoMap = &albedotex1;
    // cube.material.metallicMap = &metallictex1;
    // cube.material.roughnessMap = &roughnesstex1;
    // cube.material.normalMap = &normaltex1;
    

	Renderer renderer;
    // Cubemap skybox("../res/top.jpg", "../res/bottom.jpg", "../res/left.jpg", "../res/right.jpg", "../res/back.jpg", "../res/front.jpg");
    Cubemap skybox("../res/newport/_posy.hdr", "../res/newport/_negy.hdr", "../res/newport/_negx.hdr", "../res/newport/_posx.hdr", "../res/newport/_negz.hdr", "../res/newport/_posz.hdr");

    renderer.setSkybox(&skybox);
	std::vector<Object> objects;
	objects.push_back(suzanne);
    // objects.push_back(cube);
	// objects.push_back(plane);
    // objects.push_back(avocado);


    double lastCursorX, lastCursorY;
    int lastState = GLFW_RELEASE;
    glfwGetCursorPos(window, &lastCursorX, &lastCursorY);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (!imio.WantCaptureKeyboard) {
            if (glfwGetKey(window, GLFW_KEY_A)) {
                camera.setPosition(glm::rotateY(camera.getPosition(), -0.01f));
            }
            if (glfwGetKey(window, GLFW_KEY_D)) {
                camera.setPosition(glm::rotateY(camera.getPosition(), 0.01f));
            }
            if (glfwGetKey(window, GLFW_KEY_W)) {
                float r = glm::length(camera.getPosition());
                float r2 = r - 0.01f;
                camera.setPosition(r2/r * camera.getPosition());
            }
            if (glfwGetKey(window, GLFW_KEY_S)) {
                float r = glm::length(camera.getPosition());
                float r2 = r + 0.01f;
                camera.setPosition(r2/r * camera.getPosition());
            }
        }

        int currentState = imio.WantCaptureMouse ? GLFW_RELEASE : glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (currentState == GLFW_PRESS) {
            double currentX, currentY;
            glfwGetCursorPos(window, &currentX, &currentY);
            if (lastState == GLFW_PRESS) {
                float xdelta = currentX - lastCursorX;
                float ydelta = currentY - lastCursorY;
                glm::vec3 pos = camera.getPosition();
                pos = glm::rotateY(pos, -0.005f * xdelta);
                glm::vec3 right = glm::cross(camera.getTarget() - camera.getPosition(), glm::vec3(0.f, 1.f, 0.f));
                pos = glm::rotate(pos, -0.005f * ydelta, right);
                camera.setPosition(pos);
            }
            lastCursorX = currentX;
            lastCursorY = currentY;
        }
        lastState = currentState;

        // FINAL DRAW
        ImGui::Begin("Renderer");
		renderer.render(objects, camera);
        ImGui::Text("FPS: %.1f", imio.Framerate);
        ImGui::End();

        /* glEnable(GL_DEPTH_TEST);
        glUseProgram(skybox_program.id());
        glUniformMatrix4fv(skybox_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * glm::mat4(glm::mat3(camera.getViewMatrix())))); // remove the translation
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.id());
        glBindVertexArray(cube.mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0); */

        // === IMGUI ===
        // ImGui::ShowDemoWindow();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
