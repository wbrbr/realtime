#ifndef OBJECT_HPP
#define OBJECT_HPP
#include "material.hpp"
#include "mesh.hpp"
#include "transform.hpp"

struct Object {
    Transform transform;
    Mesh mesh;
    TextureMaterial material;
};
#endif
