#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D albedotex;
uniform sampler2D normaltex;
uniform sampler2D depthtex;
uniform sampler2D roughmettex;
uniform sampler2D positiontex;
uniform sampler2D ssaotex;
uniform samplerCube irradianceMap;


uniform vec3 camPos;
uniform vec3 lightPos;
uniform float lightStrength;

const float WIDTH = 800.0;
const float HEIGHT = 450.0;

const float PI = 3.14159265359;

vec3 fresnelShlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main()
{
    vec3 N = texture(normaltex, TexCoords).rgb;
    vec3 position = texture(positiontex, TexCoords).rgb;
    vec3 V = normalize(camPos - position);
    vec3 albedo = texture(albedotex, TexCoords).rgb;
    float roughness = texture(roughmettex, TexCoords).g;
    float metallic = texture(roughmettex, TexCoords).b;
    float opacity = texture(roughmettex, TexCoords).r;
    float ssao = texture(ssaotex, TexCoords).r;

    if (opacity < .5) discard;
    vec3 F0 = mix(vec3(0.04), albedo, metallic); 

    vec3 L0 = vec3(0.0);

    vec3 L = normalize(lightPos - position);
    vec3 H = normalize(V+L);
    float dist = length(lightPos-position);
    float attenuation = 1.0 / (dist*dist);
    vec3 radiance = vec3(lightStrength) * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelShlick(min(max(dot(H, V), 0.0), 1.0), F0);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    L0 += (kD * albedo / PI + specular) * radiance * NdotL;

    // ambient lighting (env map)
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo; // TODO: * ao
    kD = vec3(1.) - max(fresnelShlick(dot(N, V), F0), 0);
    vec3 ambient = diffuse * (1. - metallic);

    vec3 color = L0 + ambient;
    color = color / (color + vec3(1.));

    FragColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), opacity);
}
