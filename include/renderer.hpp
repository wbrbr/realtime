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
	unsigned int fbo, albedo, normal_tex, rough_met_tex, position_tex, depth_texture;

public:
	Renderer();
	void render(std::vector<Object> objects, Camera cam);
};
#endif