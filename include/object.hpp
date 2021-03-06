#ifndef OBJECT_HPP
#define OBJECT_HPP
#include "transform.hpp"
#include "mesh.hpp"
#include "material.hpp"

struct Object
{
    Transform transform;
    Mesh mesh;
	TextureMaterial material;
};
#endif
