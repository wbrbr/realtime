#include "renderer.hpp"
#include "../include/GL/gl3w.h"
#include "Tracy.hpp"
#include "TracyOpenGL.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include <iostream>
#include "glm/gtx/string_cast.hpp"
#include <random>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void doFrustumCulling(std::vector<Object>& objects, glm::mat4 world_to_clip, std::vector<Object>& remaining);

void fillNoiseTexture(unsigned int tex)
{
    glm::vec3 noise[16];
    static std::uniform_real_distribution<float> dist(0., 1.);
    static std::default_random_engine gen;
    for (unsigned int i = 0; i < 16; i++) {
        noise[i] = glm::vec3(dist(gen) * 2. - 1.,
                             dist(gen) * 2. - 1.,
                             0.);
    }

    glTextureSubImage2D(tex, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, noise);
}

unsigned int createNoiseTexture()
{
    unsigned int noise_tex;
    glGenTextures(1, &noise_tex);
    glBindTexture(GL_TEXTURE_2D, noise_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    fillNoiseTexture(noise_tex);

    return noise_tex;
}

void setupSamples(std::vector<glm::vec3>& samples)
{
    std::uniform_real_distribution<float> dist(0., 1.);
    std::default_random_engine gen;

    for (unsigned int i = 0; i < 64; i++) {
        // generate a point in the z > 0 hemisphere
        glm::vec3 sample(dist(gen) * 2. - 1.,
            dist(gen) * 2. - 1.,
            dist(gen));
        sample = glm::normalize(sample);
        sample *= dist(gen);
        samples.push_back(sample);
        // TODO: weighting on scale
    }
}

unsigned int createSkyboxVAO()
{
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    float vertices[] = {

        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    return vao;
}

Renderer::Renderer(unsigned int width, unsigned int height, TextureLoader& loader)
    : width(width)
    , height(height)
    , geometry_pass(width, height)
    , ssao_pass(width, height)
    , shading_pass(width, height)
    , taa_pass(width, height)
    , draw_program("shaders/final.vert", "shaders/draw.frag")
    , draw_depth_program("shaders/final.vert", "shaders/depthdraw.frag")
    , equirectangular_to_cubemap_program("shaders/equirectangular_to_cubemap.comp.glsl")
    , cubemap_cosine_convolution_program("shaders/cubemap_cosine_convolution.glsl")
    , skybox(nullptr)
    , irradiance{512, 512}
    , loader(&loader)
    , can_screenshot(true)
    , enable_frustrum_culling(true)
    , enable_debug_camera(false)
    , enable_aabbs(false)
{
    glGenVertexArrays(1, &dummy_vao);
}

GeometryPass::GeometryPass(unsigned int width, unsigned int height)
    : program("shaders/deferred.vert", "shaders/deferred.frag")
    , width(width)
    , height(height)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    albedo_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_tex, 0);
    normal_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_tex, 0);
    rough_met_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, rough_met_tex, 0);
    position_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, position_tex, 0);
    depth_texture = create_texture(width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GeometryPass::execute(const std::vector<Object>& objects, glm::mat4 clip_from_world, TextureLoader* loader)
{
    ZoneScopedN("Geometry Pass");
    TracyGpuZone("Geometry pass");

    // === GEOMETRY PASS ===
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program.id());
    glUniformMatrix4fv(program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(clip_from_world));
    glUniform1i(program.getLoc("albedoMap"), 0);
    glUniform1i(program.getLoc("roughnessMetallicMap"), 1);
    glUniform1i(program.getLoc("normalMap"), 2);

    for (auto obj : objects) {
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, loader->get(obj.material.albedoMap)->id());
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, loader->get(obj.material.roughnessMetallicMap)->id());
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, loader->get(obj.material.normalMap)->id());

        glBindVertexArray(obj.mesh.vao);
        glUniformMatrix4fv(program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(obj.transform.getMatrix()));

        glDrawElements(GL_TRIANGLES, obj.mesh.numIndices, GL_UNSIGNED_INT, (void*)0);
    }
}

void GeometryPass::reloadShaders()
{
    program.reload();
}

DebugDrawPass::DebugDrawPass()
    : program("shaders/debug_draw_box.vert", "shaders/debug_draw_box.frag")
{
    glCreateVertexArrays(1, &boxes_vao);
    glBindVertexArray(boxes_vao);
    glGenBuffers(1, &boxes_vertex_buf);
    glBindBuffer(GL_ARRAY_BUFFER, boxes_vertex_buf);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    unsigned int indices[] = {
        // bottom square
        0, 1,
        1, 4,
        4, 3,
        3, 0,

        // top square
        2, 6,
        6, 7,
        7, 5,
        5, 2,

        // vertical lines
        0, 2,
        1, 6,
        3, 5,
        4, 7
    };

    glGenBuffers(1, &boxes_index_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxes_index_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24 * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void DebugDrawPass::execute(const std::vector<Object>& objects, glm::mat4 clip_from_world)
{
    ZoneScopedN("Debug Draw Pass");
    TracyGpuZone("Debug Draw Pass");

    vertices.clear();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(program.id());
    glUniformMatrix4fv(program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(clip_from_world));
    glUniform3f(program.getLoc("color"), 1, 0, 0);

    for (auto obj : objects) {
        glBindVertexArray(boxes_vao);
        glNamedBufferSubData(boxes_vertex_buf, 0, 8*sizeof(glm::vec3), obj.aabb.points);
        glUniformMatrix4fv(program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(obj.transform.getMatrix()));
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void*)0);
    }
}

void DebugDrawPass::drawFrustum(glm::mat4 debug_world_to_clip, glm::mat4 active_world_to_clip)
{
    glm::mat4 debug_clip_to_world = glm::inverse(debug_world_to_clip);

    vertices.clear();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(program.id());
    glUniformMatrix4fv(program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(active_world_to_clip));
    glUniform3f(program.getLoc("color"), 0, 1, 0);

    glm::vec3 vertices[] = {
        glm::vec3(-1, -1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(-1, 1, -1),
        glm::vec3(-1, -1, 1),
        glm::vec3(1, -1, 1),
        glm::vec3(-1, 1, 1),
        glm::vec3(1, 1, -1),
        glm::vec3(1, 1, 1)
    };

    glBindVertexArray(boxes_vao);
    glNamedBufferSubData(boxes_vertex_buf, 0, 8*sizeof(glm::vec3), vertices);
    glUniformMatrix4fv(program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(debug_clip_to_world));
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void*)0);
}

void DebugDrawPass::reloadShaders()
{
    program.reload();
}

ShadowPass::ShadowPass()
    : program("shaders/depth.vert", "shaders/depth.frag")
{
    shadow_tex = create_texture(2048, 2048, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_tex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    enabled = true;
}

void ShadowPass::execute(const std::vector<Object>& objects, glm::mat4 lightMatrix)
{
    ZoneScopedN("Shadow map");
    TracyGpuZone("Shadow map");
    // === DIRECTIONAL SHADOW MAP PASS ===
    glViewport(0, 0, 2048, 2048);
    glCullFace(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(program.id());

    glUniformMatrix4fv(program.getLoc("lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightMatrix));

    for (auto obj : objects) {
        glBindVertexArray(obj.mesh.vao);
        glUniformMatrix4fv(program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(obj.transform.getMatrix()));
        glDrawElements(GL_TRIANGLES, obj.mesh.numIndices, GL_UNSIGNED_INT, (void*)0);
    }

    glCullFace(GL_BACK);
}

void ShadowPass::drawUI()
{
    if (ImGui::CollapsingHeader("Shadow mapping")) {
        ImGui::Checkbox("Enable shadow mapping", &enabled);
    }
}

void ShadowPass::reloadShaders()
{
    program.reload();
}

SSAOPass::SSAOPass(unsigned int width, unsigned int height)
    : program("shaders/final.vert", "shaders/ssao.frag")
{
    noise_tex = createNoiseTexture();
    setupSamples(samples);

    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    ssao_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_tex, 0);
    glDrawBuffers(1, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    enabled = true;
    radius = .03;
    bias = 0;
}

void SSAOPass::execute(glm::mat4 view_mat, glm::mat4 proj_mat, unsigned int normal_tex, unsigned int depth_tex, unsigned int vao)
{
    ZoneScopedN("SSAO");
    TracyGpuZone("SSAO");

    fillNoiseTexture(noise_tex);
    // === SSAO PASS ===
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // glClearColor(1., 1., 1., 1.);
    // glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program.id());
    glUniformMatrix4fv(program.getLoc("worldtoview"), 1, GL_FALSE, glm::value_ptr(view_mat));
    glUniformMatrix4fv(program.getLoc("projection"), 1, GL_FALSE, glm::value_ptr(proj_mat));
    glUniform3fv(program.getLoc("samples"), 64, reinterpret_cast<float*>(samples.data()));
    glUniform1f(program.getLoc("radius"), radius);
    glUniform1f(program.getLoc("bias"), bias);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noise_tex);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void SSAOPass::drawUI()
{
    if (ImGui::CollapsingHeader("SSAO")) {
        ImGui::Checkbox("Enable SSAO", &enabled);
        ImGui::DragFloat("Radius", &radius, 1.f, 0.f, 1.f);
        ImGui::DragFloat("Bias", &bias, 1.f, 0.f, 1.f);
    }
}

void SSAOPass::reloadShaders()
{
    program.reload();
}

ShadingPass::ShadingPass(unsigned int width, unsigned int height)
    : draw_program("shaders/final.vert", "shaders/draw.frag")
    , shading_program("shaders/final.vert", "shaders/final.frag")
    , skybox_program("shaders/skybox.vert", "shaders/skybox.frag")
{
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    shading_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shading_tex, 0);
    glDrawBuffers(1, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    cube_vao = createSkyboxVAO();

    ambient_intensity = 1.f;
    shadow_bias = 1e-8;
    use_pcf = true;
    pcf_radius = 3;

    frame_num = 0;

    skybox_choice = 0;
}

void ShadingPass::execute(glm::mat4 viewMatrix, glm::mat4 projMatrix, float zNear, float zFar, glm::vec3 lightDir, glm::mat4 lightMatrix, Cubemap* cubemap, unsigned int albedo_tex, unsigned int normal_tex, unsigned int shadow_tex, unsigned int rough_met_tex, unsigned int position_tex, unsigned int ssao_tex, const Cubemap& irradiance, unsigned int vao)
{
    ZoneScopedN("Shading pass");
    TracyGpuZone("Shading pass");

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    const Cubemap* skyboxMap = nullptr;
    switch(skybox_choice) {
        case 0:
            skyboxMap = cubemap;
            break;
        case 1:
            skyboxMap = &irradiance;
            break;

        default:
            assert(false);
    }

    if (skyboxMap) {
        glBindVertexArray(cube_vao);
        glUseProgram(skybox_program.id());
        glUniformMatrix4fv(skybox_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(projMatrix * glm::mat4(glm::mat3(viewMatrix))));
        glUniform1f(skybox_program.getLoc("scale"), 0.5f*(zNear+zFar));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxMap->id());
        glUniform1i(skybox_program.getLoc("skybox"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    static float lightPos[3] = { 4.f, 1.f, 6.f };
    static float lightColor[3] = { 1.f, 1.f, 1.f };
    if (ImGui::CollapsingHeader("Point light")) {
        ImGui::ColorEdit3("Light color", lightColor);
        ImGui::DragFloat3("Light position", lightPos, 0.001f, -10.f, 10.f);
        ImGui::Separator();
    }

    glUseProgram(shading_program.id());
    glUniform1i(shading_program.getLoc("pointLightsNum"), 1);
    glUniform3fv(shading_program.getLoc("pointLights[0].position"), 1, lightPos);
    glUniform3fv(shading_program.getLoc("pointLights[0].color"), 1, lightColor);
    glm::vec3 cameraPosition = glm::vec3(-viewMatrix[3]);
    glUniform3fv(shading_program.getLoc("camPos"), 1, glm::value_ptr(cameraPosition));
    glUniform1i(shading_program.getLoc("albedotex"), 0);
    glUniform1i(shading_program.getLoc("normaltex"), 1);
    glUniform1i(shading_program.getLoc("depthtex"), 2);
    glUniform1i(shading_program.getLoc("roughmettex"), 3);
    glUniform1i(shading_program.getLoc("positiontex"), 4);
    glUniform1i(shading_program.getLoc("ssaotex"), 5);
    glUniform1i(shading_program.getLoc("irradianceMap"), 6);
    glUniform3f(shading_program.getLoc("sunLight.dir"), lightDir.x, lightDir.y, lightDir.z);
    glUniform3f(shading_program.getLoc("sunLight.color"), 1.f, 1.f, 1.f);
    glUniformMatrix4fv(shading_program.getLoc("lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightMatrix));
    glUniform1f(shading_program.getLoc("ambientIntensity"), ambient_intensity);
    glUniform1f(shading_program.getLoc("shadowBias"), shadow_bias);
    glUniform1f(shading_program.getLoc("use_pcf"), use_pcf);
    glUniform1ui(shading_program.getLoc("frame_num"), frame_num);
    glUniform1f(shading_program.getLoc("pcf_radius"), pcf_radius);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedo_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadow_tex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, rough_met_tex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, position_tex);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, ssao_tex);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance.id());
    
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame_num++;
}

void ShadingPass::drawUI()
{
    if (ImGui::CollapsingHeader("Shading")) {
        ImGui::DragFloat("Ambient intensity", &ambient_intensity, 0.01f, 0.f, 2.f);
        ImGui::DragFloat("Shadow bias", &shadow_bias, 1.f, 0.f, 0.1f, "%.6f", 10.f);
        ImGui::Checkbox("Enable PCF", &use_pcf);
        ImGui::DragFloat("PCF radius", &pcf_radius, 1, 0, 16);
    }
    if (ImGui::CollapsingHeader("Skybox")) {
        const char* items[] = {"skybox", "irradiance"};
        ImGui::Combo("Skybox Display", &skybox_choice, items, 2);
    }
}

void ShadingPass::reloadShaders()
{
    draw_program.reload();
    shading_program.reload();
    skybox_program.reload();
}

void Renderer::render(Scene& scene, Camera& camera)
{
    ZoneScopedN("Render (CPU)");
    TracyGpuZone("Render (GPU)");

    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::DragFloat("Near plane", &camera.zNear);
        ImGui::DragFloat("Far plane", &camera.zFar);
    }

    static float lightZFar = 10.f;
    static float lightZNear = 0.1f;
    static float planeWidth = 1.f;
    static float planeHeight = 1.f;
    static glm::vec3 sunlightPosition(0.f, 10.f, 0.f);
    ImGui::Text("Position: (%f, %f, %f)", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
    if (ImGui::CollapsingHeader("Directional light")) {
        ImGui::DragFloat3("Position", glm::value_ptr(sunlightPosition));
        ImGui::DragFloat("Near plane", &lightZNear);
        ImGui::DragFloat("Far plane", &lightZFar);
        ImGui::DragFloat("Plane width", &planeWidth);
        ImGui::DragFloat("Plane height", &planeHeight);
        ImGui::Separator();
    }
    shadow_pass.drawUI();
    ssao_pass.drawUI();
    shading_pass.drawUI();
    taa_pass.drawUI();

    glm::vec3 lightDir = glm::normalize(glm::vec3(0.f, -1.f, 0.f));
    glm::vec3 up(0.f, 1.f, 0.f);
    if (glm::length(glm::cross(up, lightDir)) == 0.) {
        float sg = glm::dot(up, lightDir);
        up = sg * glm::vec3(0.f, 0.f, -1.f);
    }

    if (ImGui::CollapsingHeader("Debug view")) {
        ImGui::Checkbox("Debug Camera", &enable_debug_camera);
        ImGui::Checkbox("Bounding boxes", &enable_aabbs);
    }
    
    if (!enable_debug_camera) {
        culling_camera = camera;
    }

    glm::mat4 lightProjection = glm::ortho(-planeWidth, planeWidth, -planeHeight, planeHeight, lightZNear, lightZFar);
    glm::mat4 lightView = glm::lookAt(sunlightPosition, lightDir, up);
    glm::mat4 lightMatrix = lightProjection * lightView;

    //glm::vec2 jitter_ndc = use_taa ? glm::vec2((drand48() - 0.5) * 2.f / (float)width, (drand48() - 0.5) * 2.f / (float)height) : glm::vec2(0);
    glm::vec2 jitter_ndc = taa_pass.enabled ? ((halton.next() - glm::vec2(0.5)) * 2.f / glm::vec2((float)width, (float)height))
                                            : glm::vec2(0);
    glm::mat4 proj_mat = camera.getPerspectiveMatrix();
    /*proj_mat[2][0] = jitter.x;
    proj_mat[2][1] = jitter.y; */

    glm::mat4 jitter_mat = glm::mat4(1);
    jitter_mat[3][0] = jitter_ndc.x;
    jitter_mat[3][1] = jitter_ndc.y;

    glm::mat4 viewMatrix = camera.getViewMatrix();
    glm::mat4 projMatrix = camera.getPerspectiveMatrix();
    glm::mat4 viewProjMatrix = projMatrix * viewMatrix;

    if (enable_frustrum_culling) {
        doFrustumCulling(scene.objects, viewProjMatrix, objects_culled);
    } else {
        objects_culled = scene.objects; 
    }

    if (ImGui::CollapsingHeader("Frustum culling")) {
        ImGui::Checkbox("Enabled", &enable_frustrum_culling);
        ImGui::Text("Culled: %u", scene.objects.size() - objects_culled.size());
        ImGui::Text("Num. remaining: %u\n", objects_culled.size());
    }


    glm::mat4 clip_from_world = jitter_mat * viewProjMatrix;
    geometry_pass.execute(objects_culled, clip_from_world, loader);

    if (shadow_pass.enabled) {
        shadow_pass.execute(scene.objects, lightMatrix);
    } else {
        float val = 1.f;
        glBindFramebuffer(GL_FRAMEBUFFER, shadow_pass.fbo);
        glClearBufferfv(GL_DEPTH, 0, &val);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glViewport(0, 0, width, height);

    glDisable(GL_DEPTH_TEST);

    if (ssao_pass.enabled) {
        ssao_pass.execute(viewMatrix, jitter_mat * proj_mat, geometry_pass.normal_tex, geometry_pass.depth_texture, dummy_vao);
    } else {
        float white[] = { 1.f, 1.f, 1.f, 1.f };
        glBindFramebuffer(GL_FRAMEBUFFER, ssao_pass.fbo);
        glClearBufferfv(GL_COLOR, 0, white);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // === SHADING PASS ===
    shading_pass.execute(viewMatrix, projMatrix, camera.zNear, camera.zFar, lightDir, lightMatrix, skybox, geometry_pass.albedo_tex, geometry_pass.normal_tex, shadow_pass.shadow_tex, geometry_pass.rough_met_tex, geometry_pass.position_tex, ssao_pass.ssao_tex, irradiance, dummy_vao);

    if (taa_pass.enabled) {
        // === TAA PASS ===
        taa_pass.execute(projMatrix * viewMatrix, geometry_pass.position_tex, shading_pass.shading_tex, jitter_ndc, dummy_vao);
    }

    unsigned int final_tex = taa_pass.enabled ? taa_pass.taa_tex : shading_pass.shading_tex;

    // === DRAW TO SCREEN ===
    const char* items[] = { "final", "albedo", "normals", "depth", "roughness/metallic", "position", "ssao", "skybox", "sunlight shadow map" };
    static int current = 0;
    ImGui::Combo("Display", &current, items, 9);
    unsigned int display_tex = 0;
    switch (current) {
    case 0:
        display_tex = final_tex;
        break;
    case 1:
        display_tex = geometry_pass.albedo_tex;
        break;
    case 2:
        display_tex = geometry_pass.normal_tex;
        break;
    case 3:
        display_tex = geometry_pass.depth_texture;
        break;
    case 4:
        display_tex = geometry_pass.rough_met_tex;
        break;
    case 5:
        display_tex = geometry_pass.position_tex;
        break;
    case 6:
        display_tex = ssao_pass.ssao_tex;
        break;
    case 7:
        display_tex = shadow_pass.shadow_tex;
        break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, display_tex);
    if (current == 8) {
        glUseProgram(draw_depth_program.id());
        glUniform1f(draw_depth_program.getLoc("zNear"), lightZNear);
        glUniform1f(draw_depth_program.getLoc("zFar"), lightZFar);
    } else {
        glUseProgram(draw_program.id());
        glUniform2f(draw_program.getLoc("jitter"), jitter_ndc.x, jitter_ndc.y);
    }
    glBindVertexArray(dummy_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    if (enable_aabbs) {
		debug_draw_pass.execute(objects_culled, viewProjMatrix);
    }

    if (enable_debug_camera) {
        debug_draw_pass.drawFrustum(culling_camera.getPerspectiveMatrix() * culling_camera.getViewMatrix(), viewProjMatrix);
    }

    if (ImGui::Button("Screenshot") && can_screenshot) {
        can_screenshot = false;
        unsigned char* pixels = new unsigned char[width*height*3];
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        unsigned char* pixels_flipped = new unsigned char[width*height*3];
        for (unsigned int j = 0; j < height; j++) {
            for (unsigned int i = 0; i < width; i++) {
                unsigned int dst_index = 3*(j*width+i);
                unsigned int src_index = 3*((height-1-j)*(width)+i);
                pixels_flipped[dst_index] = pixels[src_index];
                pixels_flipped[dst_index+1] = pixels[src_index+1];
                pixels_flipped[dst_index+2] = pixels[src_index+2];
            }
        }
        stbi_write_png("screenshot.png", (int)width, (int)height, 3, pixels_flipped, (int)width*3);
        delete[] pixels;
        delete[] pixels_flipped;
    } else {
        can_screenshot = true;
    }
}

TAAPass::TAAPass(unsigned int width, unsigned int height)
    : width(width)
    , height(height)
    , program("shaders/final.vert", "shaders/taa.frag")
{
    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    taa_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, taa_tex, 0);
    glDrawBuffers(1, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float black[3] = { 0, 0, 0 };
    history_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glClearTexImage(history_tex, 0, GL_RGB, GL_FLOAT, black);

    history_clip_from_world = glm::mat4(1);

    update_history = true;
    neighborhood_clamping = true;
    enabled = true;
}

void TAAPass::execute(glm::mat4 world_to_clip, unsigned int position_tex, unsigned int shading_tex, glm::vec2 jitter, unsigned int vao)
{
    ZoneScopedN("TAA pass");
    TracyGpuZone("TAA pass");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glUseProgram(program.id());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, history_tex);
    glUniform1i(program.getLoc("history"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shading_tex);
    glUniform1i(program.getLoc("current"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, position_tex);
    glUniform1i(program.getLoc("world_positions"), 2);

    glUniformMatrix4fv(program.getLoc("history_clip_from_world"), 1, GL_FALSE, glm::value_ptr(history_clip_from_world));

    glm::vec2 pixel_size(1.f / (float)width, 1.f / (float)height);
    glUniform2f(program.getLoc("pixel_size"), pixel_size.x, pixel_size.y);

    glUniform2f(program.getLoc("jitter_ndc"), jitter.x, jitter.y);

    glUniform1i(program.getLoc("neighborhood_clamping"), neighborhood_clamping);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    history_clip_from_world = world_to_clip;

    if (update_history) {
        glCopyImageSubData(taa_tex, GL_TEXTURE_2D, 0, 0, 0, 0, history_tex, GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);
    }
}

void TAAPass::drawUI()
{
    if (ImGui::CollapsingHeader("TAA")) {
        ImGui::Checkbox("Enable TAA", &enabled);
        ImGui::Checkbox("Neighborhood clamping", &neighborhood_clamping);
        ImGui::Checkbox("Update history buffer", &update_history);
    }
}

void TAAPass::reloadShaders()
{
    program.reload();
}

void Renderer::setSkybox(Cubemap* skybox)
{
    this->skybox = skybox;
}

void Renderer::setSkyboxFromEquirectangular(const ImageTexture &texture, unsigned int width, unsigned int height)
{
    if (width % 8 != 0 || height % 8 != 0) {
        std::cerr << "Skybox width/height must be a multiple of 8" << std::endl;
        exit(1);
    }
    skybox = new Cubemap{width, height};
    glUseProgram(equirectangular_to_cubemap_program.id());
    glBindImageTexture(0, skybox->id(), 0, true, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id());
    glUniform1i(equirectangular_to_cubemap_program.getLoc("envmap"), 0);
    glUniform1i(equirectangular_to_cubemap_program.getLoc("cubemap"), 0);
    glDispatchCompute(width/8, height/8, 6);

    glUseProgram(cubemap_cosine_convolution_program.id());
    glBindImageTexture(0, irradiance.id(), 0, true, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->id());
    glGenerateTextureMipmap(skybox->id());
    glUniform1i(cubemap_cosine_convolution_program.getLoc("cubemap"), 0);
    glUniform1i(cubemap_cosine_convolution_program.getLoc("irradiance_map"), 0);

    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glDispatchCompute(width/8, height/8, 6);
}

#define MIN(a, b) ((a > b) ? a : b)

void doFrustumCulling(std::vector<Object>& objects, glm::mat4 world_to_clip, std::vector<Object>& remaining)
{
    ZoneScopedN("Frustum culling");

    remaining.clear();

    for (size_t i = 0; i < objects.size(); i++) {
        Box bounding_box = objects[i].aabb.transform(objects[i].transform.getMatrix());

        bool out_x_less = true;
        bool out_x_greater = true;
        bool out_y_less = true;
        bool out_y_greater = true;
        bool out_z_less = true;
        bool out_z_greater = true;

        for (size_t p_idx = 0; p_idx < 8; p_idx++) {
            glm::vec3 world_coords = bounding_box.points[p_idx];

            glm::vec4 clip_coords = world_to_clip * glm::vec4(world_coords, 1);

            out_x_less &= clip_coords.x < -clip_coords.w;
            out_x_greater &= clip_coords.x > clip_coords.w;
            out_y_less &= clip_coords.y < -clip_coords.w;
            out_y_greater &= clip_coords.y > clip_coords.w;
            out_z_less &= clip_coords.z < -clip_coords.w;
            out_z_greater &= clip_coords.z > clip_coords.w;
        }

        bool cull = out_x_less || out_x_greater || out_y_less || out_y_greater || out_z_less || out_z_greater; 

        if (!cull) {
            remaining.push_back(objects[i]);
        }
    }
}

void Renderer::reloadShaders()
{
    geometry_pass.reloadShaders();
    shadow_pass.reloadShaders();
    ssao_pass.reloadShaders();
    shading_pass.reloadShaders();
    taa_pass.reloadShaders();
    debug_draw_pass.reloadShaders();
}
