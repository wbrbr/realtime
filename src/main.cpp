#include "GL/gl3w.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <optional>
#include "shader.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "object.hpp"
#include "light.hpp"
#include "texture.hpp"

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
    glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 9 * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m->mNumVertices * 3 * sizeof(float), m->mVertices);
    glBufferSubData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), m->mNumVertices * 3 * sizeof(float), m->mNormals);
    if (m->HasTextureCoords(0)) {
        float* texcoords = static_cast<float*>(malloc(m->mNumVertices * 2 * sizeof(float)));
        for (unsigned int i = 0; i < m->mNumVertices; i++)
        {
            auto vec = m->mTextureCoords[0][i];
            texcoords[2*i] = vec.x;
            texcoords[2*i+1] = 1.f - vec.y;
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(m->mNumVertices * 6 * sizeof(float)));
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Object suzanne;
    suzanne.mesh = loadMesh("../meshes/suzanne2.obj").value();

    Object plane;
    plane.mesh = loadMesh("../meshes/plane.obj").value();
    plane.transform.position.y -= 0.5f;

    Object cube;
    cube.mesh = loadMesh("../meshes/skybox.obj").value();

    glEnable(GL_TEXTURE_2D);
    /* glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); */

    Camera camera;
    camera.transform.position.z += 3.f;


    Shader program("shaders/base.vert", "shaders/base.frag");
    Shader depth_program("shaders/base.vert", "shaders/depth.frag");
    // unsigned int ssao_program = loadShaderProgram("shaders/base.vert", "shaders/ssao.frag");
    Shader shadow_program("shaders/base.vert", "shaders/shadow.frag");
    Shader skybox_program("shaders/skybox.vert", "shaders/skybox.frag");
    Shader pbr_program("shaders/base.vert", "shaders/pbr.frag");
    Shader pbrtex_program("shaders/base.vert", "shaders/pbrtex.frag");

    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    unsigned int depth_texture;
    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 800, 450, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImageTexture imgtex("image.png");
    ImageTexture suzannetex("suzanne.png");
    ImageTexture albedotex("rustediron2_basecolor.png");
    ImageTexture metallictex("rustediron2_metallic.png");
    ImageTexture roughnesstex("rustediron2_roughness.png");
    Cubemap skybox("desertsky_up.tga", "desertsky_dn.tga", "desertsky_lf.tga", "desertsky_rt.tga", "desertsky_ft.tga", "desertsky_bk.tga");

    glClearColor(0.f, 0.f, 0.3f, 1.f);
    // TODO: fix this vvvvvv
    /* unsigned int model_loc = glGetUniformLocation(program, "model");
    unsigned int viewproj_loc = glGetUniformLocation(program, "viewproj");
    unsigned int light_loc = glGetUniformLocation(program, "light");
    unsigned int lightSpace_loc = glGetUniformLocation(shadow_program, "lightSpace");
    unsigned int depthTexture_loc = glGetUniformLocation(shadow_program, "depth_texture");
    unsigned int imageTexture_loc = glGetUniformLocation(shadow_program, "image_texture");
    unsigned int cameraPosition_loc = glGetUniformLocation(shadow_program, "camera_position");
    unsigned int skyboxViewproj_loc = glGetUniformLocation(skybox_program, "viewproj"); */

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

        // DEPTH PROGRAM
        glUseProgram(depth_program.id());
        glm::mat4 lightProjection = glm::ortho(-1.f, 1.f, -1.f, 1.f, 0.1f, 5.f);
        glm::mat4 lightView = glm::lookAt(glm::vec3(0.f, 1.f, 1.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 lightviewproj = lightProjection * lightView;
        glUniformMatrix4fv(depth_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(lightviewproj));
        // glUniform3f(depth_program.getLoc("light"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);

        // depth texture
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        // suzanne
        glBindVertexArray(suzanne.mesh.vao);
        glUniformMatrix4fv(depth_program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(suzanne.transform.getMatrix()));
        glDrawArrays(GL_TRIANGLES, 0, suzanne.mesh.numVertices);

        // plane
        glBindVertexArray(plane.mesh.vao);
        glUniformMatrix4fv(depth_program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(plane.transform.getMatrix()));
        glDrawArrays(GL_TRIANGLES, 0, plane.mesh.numVertices);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // SKYBOX PROGRAM
        glDepthMask(GL_FALSE);
        glUseProgram(skybox_program.id());
        glUniformMatrix4fv(skybox_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * glm::mat4(glm::mat3(camera.getViewMatrix())))); // remove the translation
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.id());
        glBindVertexArray(cube.mesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);

        /* // SHADOW PROGRAM
        glUseProgram(shadow_program.id());
        glUniformMatrix4fv(shadow_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * camera.getViewMatrix()));
        glUniformMatrix4fv(shadow_program.getLoc("lightSpace"), 1, GL_FALSE, glm::value_ptr(lightviewproj));
        glUniform3f(shadow_program.getLoc("camera_position"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
        glUniform3f(shadow_program.getLoc("light"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
        glUniform1i(shadow_program.getLoc("depth_texture"), 0);
        glUniform1i(shadow_program.getLoc("skybox"), 1);
        glUniform1i(shadow_program.getLoc("image_texture"), 2);

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, depth_texture);

        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.id());

        glActiveTexture(GL_TEXTURE0 + 2); */

        // PBR PROGRAM
        /* glUseProgram(pbr_program.id());
        glUniformMatrix4fv(pbr_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * camera.getViewMatrix()));
        glUniform3f(pbr_program.getLoc("camPos"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
        glUniform3f(pbr_program.getLoc("lightPos"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z); */

        // PBR TEX PROGRAM
        glUseProgram(pbrtex_program.id());
        glUniformMatrix4fv(pbrtex_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * camera.getViewMatrix()));
        glUniform3f(pbrtex_program.getLoc("camPos"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
        glUniform3f(pbrtex_program.getLoc("lightPos"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
        glUniform1i(pbrtex_program.getLoc("albedoMap"), 0);
        glUniform1i(pbrtex_program.getLoc("metallicMap"), 1);
        glUniform1i(pbrtex_program.getLoc("roughnessMap"), 2);

        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, albedotex.id());

        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, metallictex.id());

        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, roughnesstex.id());

        // suzanne
        // glBindTexture(GL_TEXTURE_2D, suzannetex.id());
        glBindVertexArray(suzanne.mesh.vao);
        glUniformMatrix4fv(shadow_program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(suzanne.transform.getMatrix()));
        /* glUniform3f(pbr_program.getLoc("albedo"), 1.0, 0.1, 0.1);
        glUniform1f(pbr_program.getLoc("metallic"), 0.0);
        glUniform1f(pbr_program.getLoc("roughness"), 0.7); */
        glDrawArrays(GL_TRIANGLES, 0, suzanne.mesh.numVertices);

        // plane
        // glBindTexture(GL_TEXTURE_2D, imgtex.id());
        glBindVertexArray(plane.mesh.vao);
        glUniformMatrix4fv(shadow_program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(plane.transform.getMatrix()));
        /*glUniform3f(pbr_program.getLoc("albedo"), 0.4, 0.4, 0.4);
        glUniform1f(pbr_program.getLoc("metallic"), 1.0);
        glUniform1f(pbr_program.getLoc("roughness"), 0.9); */
        glDrawArrays(GL_TRIANGLES, 0, plane.mesh.numVertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
