#include "camera.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/euler_angles.hpp"
#include <iostream>

glm::mat4 Camera::getViewMatrix() const
{
    return matrix;
}

glm::mat4 Camera::getPerspectiveMatrix() const
{
    return glm::perspective(glm::radians(60.f), 16.f / 9.f, 0.1f, 2000.f);
}

void Camera::setPosition(glm::vec3 v)
{
    pos = v;
    updateMatrix();
}

glm::vec3 Camera::getPosition() const
{
    return pos;
}

glm::vec3 Camera::getTarget() const
{
    return target;
}

void Camera::updateMatrix()
{
    matrix = glm::lookAt(pos, target, glm::vec3(0.f, 1.f, 0.f));
}

void Camera::setTarget(glm::vec3 v)
{
    target = v;
    updateMatrix();
}
