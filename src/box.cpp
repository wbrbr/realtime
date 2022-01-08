#include <glm/vec4.hpp>
#include "box.hpp"

Box Box::transform(glm::mat4 mat) const
{
    Box ret;
    for (unsigned int i = 0; i < 6; i++) {
        ret.points[i] = glm::vec3(mat * glm::vec4(points[i], 1));
    }

    return ret;
}