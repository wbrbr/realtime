#version 330 core

layout (location = 0) out vec3 albedo;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 roughness_metallic;
layout (location = 3) out vec3 position;

in vec2 TexCoords;
in vec3 Position;
in vec3 Normal;
in mat3 TBN;

void main()
{
    // === POSITION ===
    position = Position;

    // === ALBEDO ===
    albedo = vec3(0.3, 0.3, 0.3);

    // === NORMAL === 
    normal = Normal;

    // === ROUGHNESS ===
    roughness_metallic.r = 1.0;
    
    // === METALLIC ===
    roughness_metallic.g = 0.0;

    // === OPAQUE ===
    roughness_metallic.b = 1.0;
}
