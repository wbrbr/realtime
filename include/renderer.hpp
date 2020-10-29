#ifndef RENDERER_HPP
#define RENDERER_HPP
#include "object.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include <vector>

class Renderer {
public:
	Renderer(unsigned int width, unsigned int height);
	void render(std::vector<Object> objects, Camera cam);
	void setSkybox(Cubemap* skybox);

private:
	Shader deferred_program;
	Shader final_program;
	Shader ssao_program;
	Shader draw_program;
	Shader skybox_program;
	Shader depth_program;
	unsigned int fbo, albedo, normal_tex, rough_met_tex, position_tex, depth_texture, ssao_tex, noise_tex, final_tex, skybox_tex, directional_depth_tex;
	unsigned int ssao_fbo;
	unsigned int final_fbo;
	unsigned int skybox_fbo;
	unsigned int directional_depth_fbo;

	unsigned int cube_vao;
	std::vector<glm::vec3> ssao_samples;

	Cubemap* skybox;
	Cubemap irradiance;
};
#endif
