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
#include <optional>
#include "shader.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "object.hpp"

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

std::optional<Mesh> loadMesh(std::string path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
    if (!scene) {
        std::cerr << importer.GetErrorString() << std::endl;
        return {};
    }
    aiMesh* m = scene->mMeshes[0];

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 6 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m->mNumVertices * 3 * sizeof(float), m->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mNormals);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 3 * sizeof(float)));
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    Mesh mesh;
    mesh.vao = vao;
    mesh.vbo = vbo;
    mesh.numVertices = m->mNumVertices;
    return mesh;
}

int main()
{
    GLFWwindow* window = initWindow();
    if (!window)
    {
        return 1;
    }


    // int x, y, n;
    // unsigned char* data = stbi_load("image.png", &x, &y, &n, 4);

    // if (data == NULL)
    // {
    //     std::cerr << "Texture loading failed: " << stbi_failure_reason() << std::endl;
    // }


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Object suzanne;
    suzanne.mesh = loadMesh("suzanne.obj").value();

    Object plane;
    plane.mesh = loadMesh("plane.obj").value();
    plane.transform.position.y -= 0.5f;

    /* glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); */

    Camera camera;
    camera.transform.position.z += 3.f;

    unsigned int program = loadShaderProgram("shaders/base.vert", "shaders/base.frag");
    glUseProgram(program);

    glClearColor(0.f, 0.f, 0.3f, 1.f);
    unsigned int model_loc = glGetUniformLocation(program, "model");
    unsigned int viewproj_loc = glGetUniformLocation(program, "viewproj");
    unsigned int light_loc = glGetUniformLocation(program, "light");

    double lastCursorX, lastCursorY;
    glfwGetCursorPos(window, &lastCursorX, &lastCursorY);
    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_A)) {
            camera.transform.translateRelative(glm::vec3(-0.01f, 0.f, 0.f));
        }
        if (glfwGetKey(window, GLFW_KEY_D)) {
            camera.transform.translateRelative(glm::vec3(0.01f, 0.f, 0.f));
        }
        if (glfwGetKey(window, GLFW_KEY_W)) {
            camera.transform.translateRelative(glm::vec3(0.f, 0.f, -0.01f));
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            camera.transform.translateRelative(glm::vec3(0.f, 0.f, 0.01f));
        }
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            camera.transform.position.y += 0.01f;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            camera.transform.position.y -= 0.01f;
        }
        double currentCursorX, currentCursorY;
        glfwGetCursorPos(window, &currentCursorX, &currentCursorY);
        camera.transform.rotation.y -= (currentCursorX - lastCursorX) * 0.001f;
        camera.transform.rotation.x -= (currentCursorY - lastCursorY) * 0.001f;
        lastCursorX = currentCursorX;
        lastCursorY = currentCursorY;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUniformMatrix4fv(viewproj_loc, 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * camera.getViewMatrix()));
        glUniform3f(light_loc, camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
        // suzanne
        glBindVertexArray(suzanne.mesh.vao);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(suzanne.transform.getMatrix()));
        glDrawArrays(GL_TRIANGLES, 0, suzanne.mesh.numVertices);
        // plane
        glBindVertexArray(plane.mesh.vao);
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(plane.transform.getMatrix()));
        glDrawArrays(GL_TRIANGLES, 0, plane.mesh.numVertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
