#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP
#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

class Transform {
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform();
    void translateRelative(glm::vec3 vel);
    glm::mat4 getMatrix() const;
};
#endif
