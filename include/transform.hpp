#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

class Transform
{
public:
    glm::vec3 position;
    glm::vec3 rotation;

    void translateRelative(glm::vec3 vel);
    glm::mat4 getMatrix();
};
#endif
