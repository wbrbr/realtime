#ifndef RENDERER_HPP
#define RENDERER_HPP
#include "object.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include <vector>
#include "texture_loader.hpp"

class GeometryPass {
public:
    GeometryPass(unsigned int width, unsigned int height);
    void execute(const std::vector<Object>& objects, const Camera& camera, TextureLoader* loader);

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

    unsigned int shadow_tex;

private:
    Shader program;
    unsigned int fbo;
};

class SkyboxPass {
public:
    SkyboxPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, Cubemap* cubemap);

    unsigned int skybox_tex;

private:
    Shader program;
    unsigned int cube_vao;
    unsigned int fbo;
};

class SSAOPass {
public:
    SSAOPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, unsigned int position_tex, unsigned int normal_tex, unsigned int rough_met_tex);

    unsigned int ssao_tex;

private:
    Shader program;
    unsigned int fbo;
    unsigned int noise_tex;
    std::vector<glm::vec3> samples;
};

class ShadingPass {
public:
    ShadingPass(unsigned int width, unsigned int height);
    void execute(const Camera& camera, glm::vec3 lightDir, glm::mat4 lightMatrix, unsigned int skybox_tex, bool draw_skybox, unsigned int albedo_tex, unsigned int normal_tex, unsigned int shadow_tex, unsigned int rough_met_tex, unsigned int position_tex, unsigned int ssao_tex, const Cubemap& irradiance);

    unsigned int shading_tex;

private:
    unsigned int fbo;
    Shader draw_program;
    Shader shading_program;
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
    SkyboxPass skybox_pass;
    SSAOPass ssao_pass;
    ShadingPass shading_pass;
    Shader draw_program;
    Shader draw_depth_program;
    Shader taa_program;
    unsigned int noise_tex, final_tex, history_tex;
    unsigned int shading_fbo;
    unsigned int taa_fbo;

    glm::mat4 history_clip_from_world;

	unsigned int cube_vao;

	Cubemap* skybox;
	Cubemap irradiance;

	TextureLoader* loader;

    void taaPass(const Camera& camera, unsigned int position_tex, unsigned int shading_tex);
};
#endif
