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
    void execute(glm::mat4 view_mat, glm::mat4 proj_mat, unsigned int normal_tex, unsigned int rough_met_tex, unsigned int depth_tex, unsigned int vao);
    void drawUI();

    unsigned int ssao_tex;
    bool enabled;
    unsigned int fbo;

private:
    Shader program;
    unsigned int noise_tex;
    std::vector<glm::vec3> samples;

    float radius;
    float bias;
};

class ShadingPass {
public:
    ShadingPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, glm::vec3 lightDir, glm::mat4 lightMatrix, Cubemap* cubemap, unsigned int albedo_tex, unsigned int normal_tex, unsigned int shadow_tex, unsigned int rough_met_tex, unsigned int position_tex, unsigned int ssao_tex, const Cubemap& irradiance, unsigned int vao);
    void drawUI();

    unsigned int shading_tex;
    unsigned int fbo;

private:
    Shader draw_program;
    Shader shading_program;

    Shader skybox_program;
    unsigned int cube_vao;

    float ambient_intensity;
    float shadow_bias;

    bool use_pcf;
    unsigned int frame_num;
    float pcf_radius;

    int skybox_choice;
};

class TAAPass {
public:
    TAAPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, unsigned int position_tex, unsigned int shading_tex, glm::vec2 jitter, unsigned int vao);
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

struct DebugDrawPass {
public:
    DebugDrawPass();
    void execute(const std::vector<Object>& objects, glm::mat4 clip_from_world);
    void drawFrustum(const Camera& frustumCamera, const Camera& viewCamera);

private:
    Shader program;
    unsigned int boxes_vao, boxes_vertex_buf, boxes_index_buf;
    std::vector<glm::vec3> vertices;
};

class Renderer {
public:
    Renderer(unsigned int width, unsigned int height, TextureLoader& loader);
    void render(std::vector<Object> objects, Camera cam);
    void setSkybox(Cubemap* skybox);
    void setSkyboxFromEquirectangular(const ImageTexture& texture, unsigned int width, unsigned int height);

private:

    unsigned int width, height;
    GeometryPass geometry_pass;
    ShadowPass shadow_pass;
    SSAOPass ssao_pass;
    ShadingPass shading_pass;
    TAAPass taa_pass;
    DebugDrawPass debug_draw_pass;

    Shader draw_program;
    Shader draw_depth_program;
    Shader equirectangular_to_cubemap_program;
    Shader cubemap_cosine_convolution_program;

    Cubemap* skybox;
    Cubemap irradiance;

    TextureLoader* loader;

    HaltonSequence halton;

    unsigned int dummy_vao;

    bool can_screenshot;

    std::vector<Object> objects_culled; // make it a member to avoid allocating a new vector every frame
    bool enable_frustrum_culling;

    /// The camera used for frustum/occlusion culling
    /// in general it is the one passed to render()
    /// when Debug Camera is enabled, culling_camera is frozen and doesn't
    /// match the render camera anymore
    Camera culling_camera;
    bool enable_debug_camera;
};
#endif
