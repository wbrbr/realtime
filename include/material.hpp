#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include "texture.hpp"

class TextureMaterial
{
public:
	ImageTexture* albedoMap;
    ImageTexture* roughnessMetallicMap;
	ImageTexture* normalMap;
};
#endif
