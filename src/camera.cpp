#include "camera.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/quaternion.hpp"
#include <iostream>

glm::mat4 Camera::getViewMatrix()
{
    return glm::inverse(transform.getMatrix());
}

glm::mat4 Camera::getPerspectiveMatrix()
{
    return glm::perspective(glm::radians(60.f), 16.f/9.f, 0.1f, 5.f);
}
