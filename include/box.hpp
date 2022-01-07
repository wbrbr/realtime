#ifndef REALTIME_BOX_HPP
#define REALTIME_BOX_HPP
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct Box {
    glm::vec3 points[6];

    Box transform(glm::mat4 mat);
};
#endif // REALTIME_BOX_HPP