#include "renderer.hpp"
#include "../include/GL/gl3w.h"
#include "glm/gtc/type_ptr.hpp"


Renderer::Renderer(): deferred_program("shaders/deferred.vert", "shaders/deferred.frag"),
					  final_program("shaders/final.vert", "shaders/final.frag") {
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
}

void Renderer::render(std::vector<Object> objects, Camera camera) {
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

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.4f, 0.6f, 0.8f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(final_program.id());
	glUniform3f(final_program.getLoc("lightPos"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
	glUniform3f(final_program.getLoc("camPos"), camera.transform.position.x, camera.transform.position.y, camera.transform.position.z);
	glUniform1i(final_program.getLoc("albedotex"), 0);
	glUniform1i(final_program.getLoc("normaltex"), 1);
	glUniform1i(final_program.getLoc("depthtex"), 2);
	glUniform1i(final_program.getLoc("roughmettex"), 3);
	glUniform1i(final_program.getLoc("positiontex"), 4);
	glUniform1f(final_program.getLoc("zNear"), 0.1f);
	glUniform1f(final_program.getLoc("zFar"), 5.f);

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

	glDrawArrays(GL_TRIANGLES, 0, 3);
}