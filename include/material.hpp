#ifndef MATERIAL_HPP
#define MATERIAL_HPP
#include "texture.hpp"

class TextureMaterial
{
public:
	ImageTexture* albedoMap;
	ImageTexture* metallicMap;
	ImageTexture* roughnessMap;
	ImageTexture* normalMap;
};
#endif
