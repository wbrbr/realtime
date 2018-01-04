#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "transform.hpp"

class Camera {
public:
    Transform transform;

    glm::mat4 getViewMatrix();
    glm::mat4 getPerspectiveMatrix();
};
#endif
