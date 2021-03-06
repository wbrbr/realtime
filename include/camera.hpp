#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "transform.hpp"

class Camera {
public:
    glm::vec3 getPosition();
    void setPosition(glm::vec3 v);
    void setTarget(glm::vec3 v);
    glm::vec3 getTarget();
    glm::mat4 getViewMatrix();
    glm::mat4 getPerspectiveMatrix();



private:
    glm::vec3 pos;
    glm::vec3 target;
    glm::mat4 matrix;

    void updateMatrix();
};
#endif
