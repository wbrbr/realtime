#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 Position;
in vec3 Normal;

uniform vec3 camPos;
uniform vec3 lightPos;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;

const float PI = 3.14159265359;

vec3 fresnelShlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float k = (roughness + 1.0)*(roughness+1.0) / 8.0;
    float ndotv = max(dot(N,V), 0.0);
    float ndotl = max(dot(N,L), 0.0);
    float GV = ndotv/(ndotv * (1.0 - k) + k);
    float GL = ndotl/(ndotl * (1.0 - k) + k);
    return GV * GL;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float alpha2 = roughness*roughness*roughness*roughness;
    float ndoth = max(dot(N,H), 0.0);
    return alpha2/(PI * pow(ndoth * ndoth * (alpha2 - 1.0) + 1, 2.0));
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - Position);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 L0 = vec3(0.0);

    vec3 L = normalize(lightPos - Position);
    vec3 H = normalize(V+L);
    float distance = length(lightPos-Position);
    float attenuation = 1.0 / (distance*distance);
    vec3 radiance = vec3(10.0, 10.0, 10.0) * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelShlick(min(max(dot(H, V), 0.0), 1.0), F0);
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    L0 += (kD * albedo / PI + specular) * radiance * NdotL; 
    FragColor = vec4(pow(L0.r, 0.4545), pow(L0.g, 0.4545), pow(L0.b, 0.4545), 1.0);
}