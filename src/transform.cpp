#include "transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

Transform::Transform()
    : scale(1.f, 1.f, 1.f)
{
    dirty = true;
}

void Transform::translateRelative(glm::vec3 vel)
{
    glm::vec4 transformed_vel = getMatrix() * glm::vec4(vel, 0.f);
    position += glm::vec3(transformed_vel.x, transformed_vel.y, transformed_vel.z);
    dirty = true;
}

glm::mat4 Transform::getMatrix()
{
    if (dirty) {
		glm::mat4 rot_mat = glm::eulerAngleYXZ(rotation.y, rotation.x, rotation.z);
		glm::mat4 trans_mat = glm::translate(glm::mat4(), position);
		glm::mat4 scale_mat = glm::scale(glm::mat4(), scale);
		m_matrix = trans_mat * rot_mat * scale_mat;
		dirty = false;
    }
    return m_matrix;
}
