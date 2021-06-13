#ifndef HALTON_HPP
#define HALTON_HPP
#include "glm/glm.hpp"

class HaltonSequence {
public:
    HaltonSequence();

    /// Returns the next element in [0,1]^2
    glm::vec2 next();

private:
    unsigned int i;
};

#endif // HALTON_HPP
