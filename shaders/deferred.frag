#version 330 core

layout (location = 0) out vec3 albedo;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec3 roughness_metallic;
layout (location = 3) out vec3 position;

in vec2 TexCoords;
in vec3 Position;
// in vec3 Normal;
in mat3 TBN;

uniform sampler2D albedoMap;
uniform sampler2D roughnessMetallicMap;
uniform sampler2D normalMap;

void main()
{
    // === POSITION ===
    position = Position;

    // === ALBEDO ===
    albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));

    // === NORMAL === 
    vec3 N = texture(normalMap, TexCoords).rgb;
    N = normalize(N * 2.0 - 1.0);
    normal = normalize(TBN * N);

    // === ROUGHNESS ===
    roughness_metallic.g = texture(roughnessMetallicMap, TexCoords).g;
    
    // === METALLIC ===
    roughness_metallic.b = texture(roughnessMetallicMap, TexCoords).b;
    roughness_metallic.r = 1.;
}
