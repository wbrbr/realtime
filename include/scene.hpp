#ifndef SCENE_HPP
#define SCENE_HPP
#include <vector>
#include "object.hpp"

struct Scene {
    std::vector<Object> objects;
    float radius = 0;
};
#endif