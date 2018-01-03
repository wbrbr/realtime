#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera {
public:
    glm::vec3 position;
    glm::vec3 rotation;

    void translateRelative(glm::vec3 vel);
    glm::mat4 getViewMatrix();

private:
    glm::mat4 getTransform();
};
#endif
