#ifndef RENDERER_HPP
#define RENDERER_HPP
#include "object.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include <vector>
#include "texture_loader.hpp"

class Renderer {
public:
	Renderer(unsigned int width, unsigned int height, TextureLoader& loader);
	void render(std::vector<Object> objects, Camera cam);
	void setSkybox(Cubemap* skybox);

private:
	unsigned int width, height;
	Shader deferred_program;
	Shader final_program;
	Shader ssao_program;
	Shader draw_program;
	Shader skybox_program;
	Shader depth_program;
	Shader draw_depth_program;
    Shader taa_program;
    unsigned int fbo, albedo, normal_tex, rough_met_tex, position_tex, depth_texture, ssao_tex, noise_tex, final_tex, skybox_tex, directional_depth_tex, shading_tex, history_tex;
	unsigned int ssao_fbo;
    unsigned int shading_fbo;
	unsigned int skybox_fbo;
	unsigned int directional_depth_fbo;
    unsigned int taa_fbo;

    glm::mat4 history_clip_from_world;

	unsigned int cube_vao;
	std::vector<glm::vec3> ssao_samples;

	Cubemap* skybox;
	Cubemap irradiance;

	TextureLoader* loader;

	void geometryPass(const std::vector<Object>& objects, Camera& camera);
	void ssaoPass(Camera& camera);
	void shadowPass(const std::vector<Object>& objects, glm::mat4 lightMatrix);
	void skyboxPass(Camera& camera);
    void shadingPass(Camera& camera, glm::vec3 lightDir, glm::mat4 lightMatrix);
    void taaPass(const Camera& camera);
};
#endif
