#include "transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

void Transform::translateRelative(glm::vec3 vel)
{
    glm::vec4 transformed_vel = getMatrix() * glm::vec4(vel, 0.f);
    position += glm::vec3(transformed_vel.x, transformed_vel.y, transformed_vel.z);
}

glm::mat4 Transform::getMatrix()
{
    auto rot_mat = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
    auto trans_mat = glm::translate(glm::mat4(), position);
    return trans_mat * rot_mat;
}
