#include "GL/gl3w.h"
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.hpp"
#include "camera.hpp"

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

    GLFWwindow* window = glfwCreateWindow(800, 450, "Real-time rendering", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return nullptr;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

int main()
{
    GLFWwindow* window = initWindow();
    if (!window)
    {
        return 1;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("mesh.obj", aiProcess_Triangulate);
    if (!scene) {
        std::cerr << importer.GetErrorString() << std::endl;
        return 1;
    }
    aiMesh* mesh = scene->mMeshes[0];

    // int x, y, n;
    // unsigned char* data = stbi_load("image.png", &x, &y, &n, 4);

    // if (data == NULL)
    // {
    //     std::cerr << "Texture loading failed: " << stbi_failure_reason() << std::endl;
    // }

    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 6 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mesh->mNumVertices * 3 * sizeof(float), mesh->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, mesh->mNumVertices * 3 * sizeof(float), mesh->mNumVertices * 3 * sizeof(float), mesh->mNormals);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(mesh->mNumVertices * 3 * sizeof(float)));
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    /* glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); */

    Camera camera;
    camera.position.z += 3.f;

    auto model = glm::mat4();
    model = glm::rotate(model, glm::radians(30.f), glm::vec3(0.f, 1.f, 0.f));
    // auto view = glm::mat4();
    // view = glm::translate(view, glm::vec3(0.f, 0.f, -3.f));
    auto proj = glm::perspective(glm::radians(60.f), 16.f/9.f, 0.1f, 10.f);

    unsigned int program = loadShaderProgram("shaders/base.vert", "shaders/base.frag");
    glUseProgram(program);

    glClearColor(0.f, 0.f, 0.3f, 1.f);
    unsigned int mvp_loc = glGetUniformLocation(program, "mvp");
    unsigned int light_loc = glGetUniformLocation(program, "light");

    double lastCursorX, lastCursorY;
    glfwGetCursorPos(window, &lastCursorX, &lastCursorY);
    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_A)) {
            camera.translateRelative(glm::vec3(-0.01f, 0.f, 0.f));
        }
        if (glfwGetKey(window, GLFW_KEY_D)) {
            camera.translateRelative(glm::vec3(0.01f, 0.f, 0.f));
        }
        if (glfwGetKey(window, GLFW_KEY_W)) {
            camera.translateRelative(glm::vec3(0.f, 0.f, -0.01f));
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            camera.translateRelative(glm::vec3(0.f, 0.f, 0.01f));
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            camera.position.y += 0.01f;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            camera.position.y -= 0.01f;
        }
        double currentCursorX, currentCursorY;
        glfwGetCursorPos(window, &currentCursorX, &currentCursorY);
        camera.rotation.y -= (currentCursorX - lastCursorX) * 0.001f;
        camera.rotation.x -= (currentCursorY - lastCursorY) * 0.001f;
        lastCursorX = currentCursorX;
        lastCursorY = currentCursorY;
        auto view = camera.getViewMatrix();
        auto mvp = proj * view * model;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3f(light_loc, 0.f, 0.f, 3.f);
        glDrawArrays(GL_TRIANGLES, 0, mesh->mNumVertices);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
