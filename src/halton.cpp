#include "halton.hpp"
#include <iostream>

constexpr glm::vec2 sequence[] = {
    glm::vec2(1. / 2., 1. / 3.),
    glm::vec2(1. / 4., 2. / 3.),
    glm::vec2(3. / 4., 1. / 9.),
    glm::vec2(1. / 8., 4. / 9.),
    glm::vec2(5. / 8., 7. / 9.),
    glm::vec2(3. / 8., 2. / 9.),
    glm::vec2(7. / 8., 5. / 9.),
    glm::vec2(1. / 16., 8. / 9.),
};

HaltonSequence::HaltonSequence()
{
    i = 0;
}

glm::vec2 HaltonSequence::next()
{
    glm::vec2 v = sequence[i];
    i = (i + 1) % 8;
    return v;
}
