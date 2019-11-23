#include "renderer.hpp"
#include "../include/GL/gl3w.h"
#include "glm/gtc/type_ptr.hpp"
#include <random>
#include "imgui.h"

unsigned int createNoiseTexture()
{
	unsigned int noise_tex;
	glGenTextures(1, &noise_tex);
	std::vector<glm::vec3> noise;
	std::uniform_real_distribution<float> dist(0., 1.);
	std::default_random_engine gen;
	for (unsigned int i = 0; i < 16; i++)
	{
		noise.push_back(glm::vec3(dist(gen) * 2. - 1.,
								  dist(gen) * 2. - 1.,
								  0.));
	}
	glGenTextures(1, &noise_tex);
	glBindTexture(GL_TEXTURE_2D, noise_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return noise_tex;
}

void setupSamples(std::vector<glm::vec3>& samples)
{
	std::uniform_real_distribution<float> dist(0., 1.);
	std::default_random_engine gen;

	for (unsigned int i = 0; i < 64; i++)
	{
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
      
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	return vao;
}

Renderer::Renderer(): deferred_program("../shaders/deferred.vert", "../shaders/deferred.frag"),
					  final_program("../shaders/final.vert", "../shaders/final.frag"),
					  ssao_program("../shaders/final.vert", "../shaders/ssao.frag"),
					  draw_program("../shaders/final.vert", "../shaders/draw.frag"),
					  skybox_program("../shaders/skybox.vert", "../shaders/skybox.frag"),
					  skybox(nullptr),
					  irradiance("../res/newport/irr_posy.hdr", "../res/newport/irr_negy.hdr", "../res/newport/irr_negx.hdr", "../res/newport/irr_posx.hdr", "../res/newport/irr_negz.hdr", "../res/newport/irr_posz.hdr") {
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	albedo = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo, 0);
	normal_tex = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_tex, 0);
	rough_met_tex = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, rough_met_tex, 0);
	position_tex = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, position_tex, 0);
	depth_texture = create_texture(800, 450, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &ssao_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
	ssao_tex = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_tex, 0);
	glDrawBuffers(1, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &final_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
	final_tex = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, final_tex, 0);
	glDrawBuffers(1, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &skybox_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, skybox_fbo);
	skybox_tex = create_texture(800, 450, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skybox_tex, 0);
	glDrawBuffers(1, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	noise_tex = createNoiseTexture();
	setupSamples(ssao_samples);

	cube_vao = createSkyboxVAO();
}

void Renderer::render(std::vector<Object> objects, Camera camera) {

	// === GEOMETRY PASS ===
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(deferred_program.id());
	glUniformMatrix4fv(deferred_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * camera.getViewMatrix()));
	glUniform1i(deferred_program.getLoc("albedoMap"), 0);
	glUniform1i(deferred_program.getLoc("metallicMap"), 1);
	glUniform1i(deferred_program.getLoc("roughnessMap"), 2);
	glUniform1i(deferred_program.getLoc("normalMap"), 3);

	for (auto obj : objects) {
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, obj.material.albedoMap->id());
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, obj.material.metallicMap->id());
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, obj.material.roughnessMap->id());
		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, obj.material.normalMap->id());

		glBindVertexArray(obj.mesh.vao);
		glUniformMatrix4fv(deferred_program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(obj.transform.getMatrix()));
		glDrawArrays(GL_TRIANGLES, 0, obj.mesh.numVertices);
	}
	
	glDisable(GL_DEPTH_TEST);

	// === SSAO PASS ===
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
	// glClearColor(1., 1., 1., 1.);
	// glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(ssao_program.id());
	glUniform1i(ssao_program.getLoc("positiontex"), 0);
	glUniform1i(ssao_program.getLoc("normaltex"), 1);
	glUniform1i(ssao_program.getLoc("noisetex"), 2);
	glUniform1i(ssao_program.getLoc("roughmettex"), 3);
	glUniformMatrix4fv(ssao_program.getLoc("worldtoview"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
	glUniformMatrix4fv(ssao_program.getLoc("projection"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix()));
	glUniform3fv(ssao_program.getLoc("samples"), 64, reinterpret_cast<float*>(ssao_samples.data()));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, position_tex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal_tex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noise_tex);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, rough_met_tex);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// === SKYBOX PASS ===
	// glBindVertexArray(0);
	if (skybox != nullptr) {
		glBindVertexArray(cube_vao);
		glBindFramebuffer(GL_FRAMEBUFFER, skybox_fbo);
		glUseProgram(skybox_program.id());
		glUniformMatrix4fv(skybox_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix()*glm::mat4(glm::mat3(camera.getViewMatrix()))));
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->id());
		glDrawArrays(GL_TRIANGLES, 0, 36);
		// glBindVertexArray(0);
	}

	// === FINAL PASS ===
	glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
	// glClearColor(0.4f, 0.6f, 0.8f, 1.f);
	// glClear(GL_COLOR_BUFFER_BIT);

	if (skybox != nullptr) {
		glUseProgram(draw_program.id());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skybox_tex);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	
	glUseProgram(final_program.id());
	static float lightPos[3] = {1.f, 1.f, 1.f};
	ImGui::DragFloat3("Light position", lightPos, 0.001f, -10.f, 10.f);
	glUniform3f(final_program.getLoc("lightPos"), 1.f, 1.f, 1.f);
	glUniform3f(final_program.getLoc("camPos"), camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
	glUniform1i(final_program.getLoc("albedotex"), 0);
	glUniform1i(final_program.getLoc("normaltex"), 1);
	glUniform1i(final_program.getLoc("depthtex"), 2);
	glUniform1i(final_program.getLoc("roughmettex"), 3);
	glUniform1i(final_program.getLoc("positiontex"), 4);
	glUniform1i(final_program.getLoc("ssaotex"), 5);
	glUniform1i(final_program.getLoc("irradianceMap"), 6);
	static float lightStrength = 1.f;
	ImGui::DragFloat("Light strength", &lightStrength, 0.01f, 0.f, 10.f);
	glUniform1f(final_program.getLoc("lightStrength"), lightStrength);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedo);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal_tex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, depth_texture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, rough_met_tex);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, position_tex);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, ssao_tex);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance.id());

	glDrawArrays(GL_TRIANGLES, 0, 3);

	// === DRAW TO SCREEN ===
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(draw_program.id());
	// glClearColor(0.f, 0.f, 0.f, 1.f);
	// glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	const char* items[] = { "final", "albedo", "normals", "depth", "roughness/metallic", "position", "ssao", "skybox"};
	static int current = 0;
	ImGui::Combo("Display", &current, items, 8);
	unsigned int display_tex = 0;
	switch (current) {
		case 0: display_tex = final_tex; break;
		case 1: display_tex = albedo; break;
		case 2: display_tex = normal_tex; break;
		case 3: display_tex = depth_texture; break;
		case 4: display_tex = rough_met_tex; break;
		case 5: display_tex = position_tex; break;
		case 6: display_tex = ssao_tex; break;
		case 7: display_tex = skybox_tex; break;
	}
	glBindTexture(GL_TEXTURE_2D, display_tex);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::setSkybox(Cubemap* skybox)
{
	this->skybox = skybox;
}