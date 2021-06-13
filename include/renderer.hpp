#ifndef RENDERER_HPP
#define RENDERER_HPP
#include "camera.hpp"
#include "halton.hpp"
#include "object.hpp"
#include "shader.hpp"
#include "texture_loader.hpp"
#include <vector>

class GeometryPass {
public:
    GeometryPass(unsigned int width, unsigned int height);
    void execute(const std::vector<Object>& objects, glm::mat4 clip_from_world, TextureLoader* loader);

    unsigned int albedo_tex;
    unsigned int normal_tex;
    unsigned int rough_met_tex;
    unsigned int position_tex;
    unsigned int depth_texture;

private:
    Shader program;
    unsigned int fbo;
    unsigned int width, height;
};

class ShadowPass {
public:
    ShadowPass();
    void execute(const std::vector<Object>& objects, glm::mat4 lightMatrix);
    void drawUI();

    unsigned int shadow_tex;
    bool enabled;
    unsigned int fbo;

private:
    Shader program;
};

class SSAOPass {
public:
    SSAOPass(unsigned int width, unsigned int height);
    void execute(glm::mat4 view_mat, glm::mat4 proj_mat, unsigned int position_tex, unsigned int normal_tex, unsigned int rough_met_tex);
    void drawUI();

    unsigned int ssao_tex;
    bool enabled;
    unsigned int fbo;

private:
    Shader program;
    unsigned int noise_tex;
    std::vector<glm::vec3> samples;
};

class ShadingPass {
public:
    ShadingPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, glm::vec3 lightDir, glm::mat4 lightMatrix, Cubemap* cubemap, unsigned int albedo_tex, unsigned int normal_tex, unsigned int shadow_tex, unsigned int rough_met_tex, unsigned int position_tex, unsigned int ssao_tex, const Cubemap& irradiance);

    unsigned int shading_tex;
    unsigned int fbo;

private:
    Shader draw_program;
    Shader shading_program;

    Shader skybox_program;
    unsigned int cube_vao;
};

class TAAPass {
public:
    TAAPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, unsigned int position_tex, unsigned int shading_tex, glm::vec2 jitter);
    void drawUI();

    bool enabled;
    unsigned int taa_tex;
    unsigned int history_tex;

private:
    unsigned int width, height;
    unsigned int fbo;
    Shader program;
    glm::mat4 history_clip_from_world;

    bool update_history;
    bool neighborhood_clamping;
};

class Renderer {
public:
    Renderer(unsigned int width, unsigned int height, TextureLoader& loader);
    void render(std::vector<Object> objects, Camera cam);
    void setSkybox(Cubemap* skybox);

private:
    unsigned int width, height;
    GeometryPass geometry_pass;
    ShadowPass shadow_pass;
    SSAOPass ssao_pass;
    ShadingPass shading_pass;
    TAAPass taa_pass;
    Shader draw_program;
    Shader draw_depth_program;

    Cubemap* skybox;
    Cubemap irradiance;

    TextureLoader* loader;

    HaltonSequence halton;
};
#endif
