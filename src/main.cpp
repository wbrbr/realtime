#include "GL/gl3w.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "shader.hpp"

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

    float vertices[] = {
        -0.5f, -0.5f, 0.f,
        -0.5f, 0.5f, 0.f,
        0.5f, 0.5f, 0.f,
        0.5f, 0.5f, 0.f,
        0.5f, -0.5f, 0.f,
        -0.5f, -0.5f, 0.f,
    };

    std::cout << glGetString(GL_VERSION) << ":" << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    unsigned int program = loadShaderProgram("shaders/base.vert", "shaders/base.frag");
    glUseProgram(program);

    glClearColor(0.f, 0.f, 0.3f, 1.f);
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
