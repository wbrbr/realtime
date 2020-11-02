#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include "texture_loader.hpp"

class TextureMaterial
{
public:
	TexID albedoMap;
    TexID roughnessMetallicMap;
	TexID normalMap;
};
#endif
