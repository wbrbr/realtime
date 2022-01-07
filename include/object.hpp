#ifndef OBJECT_HPP
#define OBJECT_HPP
#include "box.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "transform.hpp"

struct Object {
    Transform transform;
    Mesh mesh;
    TextureMaterial material;
    Box aabb;
};
#endif
