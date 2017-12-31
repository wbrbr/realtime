#include "GL/gl3w.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLFWwindow* initWindow()
{
    if (!glfwInit())
    {
        return nullptr;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 450, "Real-time rendering", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return nullptr;
    }
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
    std::cout << scene->mNumMeshes << std::endl;
    aiMesh* mesh = scene->mMeshes[0];
    std::cout << mesh->mName.C_Str() << std::endl;
    std::cout << mesh->mNumVertices << std::endl;

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
    glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 3 * sizeof(float), mesh->mVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    // glEnableVertexAttribArray(1);

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

    auto model = glm::mat4();
    model = glm::rotate(model, glm::radians(30.f), glm::vec3(0.f, 1.f, 0.f));
    auto view = glm::mat4();
    view = glm::translate(view, glm::vec3(0.f, 0.f, -5.f));
    auto proj = glm::perspective(glm::radians(75.f), 16.f/9.f, 0.1f, 10.f);
    auto mvp = proj * view * model; // TODO: not true

    unsigned int program = loadShaderProgram("shaders/base.vert", "shaders/base.frag");
    glUseProgram(program);

    glClearColor(0.f, 0.f, 0.3f, 1.f);
    unsigned int mvp_loc = glGetUniformLocation(program, "mvp");
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glDrawArrays(GL_TRIANGLES, 0, mesh->mNumVertices);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
