#ifndef RENDERER_HPP
#define RENDERER_HPP
#include "object.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include <vector>

class Renderer {
private:
	Shader deferred_program;
	Shader final_program;
	Shader ssao_program;
	unsigned int fbo, albedo, normal_tex, rough_met_tex, position_tex, depth_texture, ssao_tex, noise_tex;
	unsigned int ssao_fbo;
	std::vector<glm::vec3> ssao_samples;

public:
	Renderer();
	void render(std::vector<Object> objects, Camera cam);
};
#endif