#include "renderer.hpp"
#include "../include/GL/gl3w.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Tracy.hpp"
#include "TracyOpenGL.hpp"
#include <random>
#include <iostream>
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

Renderer::Renderer(unsigned int width, unsigned int height, TextureLoader& loader):
				      width(width),
					  height(height),
                      geometry_pass(width, height),
                      final_program("shaders/final.vert", "shaders/final.frag"),
					  ssao_program("shaders/final.vert", "shaders/ssao.frag"),
					  draw_program("shaders/final.vert", "shaders/draw.frag"),
					  skybox_program("shaders/skybox.vert", "shaders/skybox.frag"),
					  depth_program("shaders/depth.vert", "shaders/depth.frag"),
                      draw_depth_program("shaders/final.vert", "shaders/depthdraw.frag"),
                      taa_program("shaders/final.vert", "shaders/taa.frag"),
					  skybox(nullptr),
					  irradiance("res/newport/irr_posy.hdr", "res/newport/irr_negy.hdr", "res/newport/irr_negx.hdr", "res/newport/irr_posx.hdr", "res/newport/irr_negz.hdr", "res/newport/irr_posz.hdr"),
					  loader(&loader) {

    unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };

    glGenFramebuffers(1, &ssao_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
	ssao_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_tex, 0);
	glDrawBuffers(1, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &shading_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shading_fbo);
    shading_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shading_tex, 0);
    glDrawBuffers(1, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &taa_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, taa_fbo);
	final_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, final_tex, 0);
	glDrawBuffers(1, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &skybox_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, skybox_fbo);
	skybox_tex = create_texture(width, height, GL_RGB32F, GL_RGB);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skybox_tex, 0);
	glDrawBuffers(1, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	noise_tex = createNoiseTexture();
	setupSamples(ssao_samples);

	directional_depth_tex = create_texture(2048, 2048, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT);

    history_tex = create_texture(width, height, GL_RGB32F, GL_RGB);

	glGenFramebuffers(1, &directional_depth_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, directional_depth_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, directional_depth_tex, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	cube_vao = createSkyboxVAO();
}

GeometryPass::GeometryPass(unsigned int width, unsigned int height): program("shaders/deferred.vert", "shaders/deferred.frag"), width(width), height(height)
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

void GeometryPass::execute(const std::vector<Object>& objects, const Camera& camera, TextureLoader* loader)
{
	ZoneScopedN("Geometry Pass")
	TracyGpuZone("Geometry pass")

	// === GEOMETRY PASS ===
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float pixel_width = 2.f / (float)width;
    float pixel_height = 2.f / (float)height;
    glm::mat4 jitter_mat = glm::mat4(1);
    jitter_mat[3][0] += (drand48() - 0.5) * pixel_width;
    jitter_mat[3][1] += (drand48() - 0.5) * pixel_height;

    glUseProgram(program.id());
    glUniformMatrix4fv(program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(jitter_mat * camera.getPerspectiveMatrix() * camera.getViewMatrix()));
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

void Renderer::shadowPass(const std::vector<Object>& objects, glm::mat4 lightMatrix)
{
	ZoneScopedN("Shadow map")
	TracyGpuZone("Shadow map")
	// === DIRECTIONAL SHADOW MAP PASS ===
	glViewport(0, 0, 2048, 2048);
	glCullFace(GL_FRONT);
	glBindFramebuffer(GL_FRAMEBUFFER, directional_depth_fbo);
	glClear(GL_DEPTH_BUFFER_BIT);


	glUseProgram(depth_program.id());

	glUniformMatrix4fv(depth_program.getLoc("lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightMatrix));

	for (auto obj : objects) {
		glBindVertexArray(obj.mesh.vao);
		glUniformMatrix4fv(depth_program.getLoc("model"), 1, GL_FALSE, glm::value_ptr(obj.transform.getMatrix()));
		glDrawElements(GL_TRIANGLES, obj.mesh.numIndices, GL_UNSIGNED_INT, (void*)0);
	}

	glViewport(0, 0, width, height);
	glCullFace(GL_BACK);
}

void Renderer::ssaoPass(Camera& camera, unsigned int position_tex, unsigned int normal_tex, unsigned int rough_met_tex)
{
	ZoneScopedN("SSAO")
	TracyGpuZone("SSAO")
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
}

void Renderer::skyboxPass(Camera& camera)
{
	ZoneScopedN("Skybox")
	TracyGpuZone("Skybox")

	glBindVertexArray(cube_vao);
	glBindFramebuffer(GL_FRAMEBUFFER, skybox_fbo);
	glUseProgram(skybox_program.id());
	glUniformMatrix4fv(skybox_program.getLoc("viewproj"), 1, GL_FALSE, glm::value_ptr(camera.getPerspectiveMatrix() * glm::mat4(glm::mat3(camera.getViewMatrix()))));
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->id());
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::shadingPass(Camera& camera, glm::vec3 lightDir, glm::mat4 lightMatrix)
{
    ZoneScopedN("Shading pass")
    TracyGpuZone("Shading pass")

    glBindFramebuffer(GL_FRAMEBUFFER, shading_fbo);

	if (skybox != nullptr) {
		glUseProgram(draw_program.id());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skybox_tex);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	
	static float lightPos[3] = {4.f, 1.f, 6.f };
	static float lightColor[3] = {1.f, 1.f, 1.f};
	if (ImGui::CollapsingHeader("Point light")) {
		ImGui::ColorEdit3("Light color", lightColor);
		ImGui::DragFloat3("Light position", lightPos, 0.001f, -10.f, 10.f);
		ImGui::Separator();
	}
	static float ambientIntensity = 1.f;
	ImGui::DragFloat("Ambient intensity", &ambientIntensity, 0.01f, 0.f, 2.f);
	static float shadowBias = 0.001f;
	ImGui::SliderFloat("Shadow bias", &shadowBias, 0.f, 0.1f, "%.6f", 10.f);


	glUseProgram(final_program.id());
	glUniform1i(final_program.getLoc("pointLightsNum"), 1);
	glUniform3fv(final_program.getLoc("pointLights[0].position"), 1, lightPos);
	glUniform3fv(final_program.getLoc("pointLights[0].color"), 1, lightColor);
	glUniform3f(final_program.getLoc("camPos"), camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
	glUniform1i(final_program.getLoc("albedotex"), 0);
	glUniform1i(final_program.getLoc("normaltex"), 1);
	glUniform1i(final_program.getLoc("depthtex"), 2);
	glUniform1i(final_program.getLoc("roughmettex"), 3);
	glUniform1i(final_program.getLoc("positiontex"), 4);
	glUniform1i(final_program.getLoc("ssaotex"), 5);
	glUniform1i(final_program.getLoc("irradianceMap"), 6);
	glUniform3f(final_program.getLoc("sunLight.dir"), lightDir.x, lightDir.y, lightDir.z);
	glUniform3f(final_program.getLoc("sunLight.color"), 1.f, 1.f, 1.f);
	glUniformMatrix4fv(final_program.getLoc("lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightMatrix));
	glUniform1f(final_program.getLoc("ambientIntensity"), ambientIntensity);
	glUniform1f(final_program.getLoc("shadowBias"), shadowBias);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, geometry_pass.albedo_tex);
	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, geometry_pass.normal_tex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, directional_depth_tex);
	glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, geometry_pass.rough_met_tex);
	glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, geometry_pass.position_tex);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, ssao_tex);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance.id());

	glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render(std::vector<Object> objects, Camera camera) {
	ZoneScopedN("Render (CPU)")
	TracyGpuZone("Render (GPU)")

	static float zFar = 10.f;
	static float zNear = 0.1f;
	static float planeWidth = 1.f;
	static float planeHeight = 1.f;
	static glm::vec3 sunlightPosition(0.f, 10.f, 0.f);
	if (ImGui::CollapsingHeader("Directional light")) {
		ImGui::DragFloat3("Position", glm::value_ptr(sunlightPosition));
		ImGui::DragFloat("Near plane", &zNear);
		ImGui::DragFloat("Far plane", &zFar);
		ImGui::DragFloat("Plane width", &planeWidth);
		ImGui::DragFloat("Plane height", &planeHeight);
		ImGui::Separator();
	}
	glm::vec3 lightDir = glm::normalize(glm::vec3(0.f, -1.f, 0.f));
	glm::vec3 up(0.f, 1.f, 0.f);
	if (glm::length(glm::cross(up, lightDir)) == 0.) {
		float sg = glm::dot(up, lightDir);
		up = sg * glm::vec3(0.f, 0.f, -1.f);
	}

	glm::mat4 lightProjection = glm::ortho(-planeWidth, planeWidth, -planeHeight, planeHeight, zNear, zFar);
	glm::mat4 lightView = glm::lookAt(sunlightPosition, lightDir, up);
	glm::mat4 lightMatrix = lightProjection * lightView;

	ImGui::Text("Position: (%f, %f, %f)", camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);

    geometry_pass.execute(objects, camera, loader);
	shadowPass(objects, lightMatrix);
    ssaoPass(camera, geometry_pass.position_tex, geometry_pass.normal_tex, geometry_pass.rough_met_tex);

	if (skybox != nullptr) {
		skyboxPass(camera);
	}

    // === SHADING PASS ===
    shadingPass(camera, lightDir, lightMatrix);

    // === TAA PASS ===
    taaPass(camera, geometry_pass.position_tex);

	// === DRAW TO SCREEN ===
	const char* items[] = { "final", "albedo", "normals", "depth", "roughness/metallic", "position", "ssao", "skybox", "sunlight shadow map"};
	static int current = 0;
	ImGui::Combo("Display", &current, items, 9);
	unsigned int display_tex = 0;
	switch (current) {
		case 0: display_tex = final_tex; break;
        case 1: display_tex = geometry_pass.albedo_tex; break;
        case 2: display_tex = geometry_pass.normal_tex; break;
        case 3: display_tex = geometry_pass.depth_texture; break;
        case 4: display_tex = geometry_pass.rough_met_tex; break;
        case 5: display_tex = geometry_pass.position_tex; break;
		case 6: display_tex = ssao_tex; break;
		case 7: display_tex = skybox_tex; break;
		case 8: display_tex = directional_depth_tex; break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, display_tex);
	if (current == 8) {
		glUseProgram(draw_depth_program.id());
		glUniform1f(draw_depth_program.getLoc("zNear"), zNear);
		glUniform1f(draw_depth_program.getLoc("zFar"), zFar);
	} else {	
		glUseProgram(draw_program.id());
	}
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::taaPass(const Camera& camera, unsigned int position_tex)
{
    glBindFramebuffer(GL_FRAMEBUFFER, taa_fbo);

    glUseProgram(taa_program.id());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, history_tex);
    glUniform1i(taa_program.getLoc("history"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shading_tex);
    glUniform1i(taa_program.getLoc("current"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, position_tex);
    glUniform1i(taa_program.getLoc("world_positions"), 2);

    glUniformMatrix4fv(taa_program.getLoc("history_clip_from_world"), 1, GL_FALSE, glm::value_ptr(history_clip_from_world));

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    history_clip_from_world = camera.getPerspectiveMatrix() * camera.getViewMatrix();

    // copy frame to history
    glCopyImageSubData(final_tex, GL_TEXTURE_2D, 0, 0, 0, 0, history_tex, GL_TEXTURE_2D, 0, 0, 0, 0, width, height, 1);
}

void Renderer::setSkybox(Cubemap* skybox)
{
	this->skybox = skybox;
}
