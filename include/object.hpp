#ifndef OBJECT_HPP
#define OBJECT_HPP
#include "transform.hpp"
#include "mesh.hpp"

struct Object
{
    Transform transform;
    Mesh mesh;
};
#endif
