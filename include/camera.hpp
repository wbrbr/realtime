#ifndef CAMERA_HPP
#define CAMERA_HPP
#include "transform.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    Camera();
    glm::vec3 getPosition() const;
    void setPosition(glm::vec3 v);
    void setTarget(glm::vec3 v);
    glm::vec3 getTarget() const;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getPerspectiveMatrix() const;

    float zNear;
    float zFar;

private:
    glm::vec3 pos;
    glm::vec3 target;
    glm::mat4 matrix;

    void updateMatrix();
};
#endif
