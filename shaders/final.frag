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

uniform struct PointLight {
    vec3 position;
    vec3 color;
} pointLights[32];

uniform int pointLightsNum;

uniform struct DirectionalLight {
    vec3 dir;
    vec3 color;
} sunLight;

uniform vec3 camPos;
uniform mat4 lightSpaceMatrix;

uniform float ambientIntensity;
uniform float shadowBias;

uniform bool use_pcf;
uniform uint frame_num;
uniform float pcf_radius;

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

vec3 shade(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metallic)
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic); 
    vec3 H = normalize(V+L);
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
    return (kD*albedo / PI + specular) * NdotL;
}

// halton sequence in [0,1]^2
const vec2 halton[8] = vec2[](
    vec2(1. / 2., 1. / 3.),
    vec2(1. / 4., 2. / 3.),
    vec2(3. / 4., 1. / 9.),
    vec2(1. / 8., 4. / 9.),
    vec2(5. / 8., 7. / 9.),
    vec2(3. / 8., 2. / 9.),
    vec2(7. / 8., 5. / 9.),
    vec2(1. / 16., 8. / 9.)
);

const vec2 sampling_pattern[4] = vec2[](
    vec2(-1,-1),
    vec2(-1,1),
    vec2(1,-1),
    vec2(1,1)
);

uvec3 pcg3d(uvec3 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    v = v ^ (v >> 16u);
    v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
    return v;
}

float shadow(vec3 position)
{
    vec4 lightSpacePos = lightSpaceMatrix * vec4(position, 1.);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * .5 + .5;
    float depth = projCoords.z;

    float shadowFactor = 0.0;

    if (use_pcf) {

        uvec3 px_coord = uvec3(uvec2(gl_FragCoord.xy), frame_num);

        uvec3 rnd = pcg3d(px_coord);

        float x_flip = rnd.x % 2u == 0u ? 1 : -1;
        float y_flip = rnd.y % 2u == 0u ? 1 : -1;

        for (int i = 0; i < 8; i++)
        {
            vec2 px_offset = 2 * (halton[i] - 0.5);
            px_offset *= pcf_radius;
            px_offset.x *= x_flip;
            px_offset.y *= y_flip;

            vec2 uv = projCoords.xy + px_offset / 2048; // TODO: take shadow map resolution as uniform
            float closestDepth = texture(depthtex, uv).r;
            if (depth <= closestDepth + shadowBias) {
                shadowFactor += 1.0;
            }
        }

        shadowFactor /= 8.0;
    } else {
        float closestDepth = texture(depthtex, projCoords.xy).r;
        if (depth <= closestDepth + shadowBias) {
            shadowFactor = 1;
        }
    }

    return shadowFactor;
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

    vec3 L0 = vec3(0.0);
    
    for (int i = 0; i < pointLightsNum; i++)
    {
        vec3 L = normalize(pointLights[i].position - position);
        float dist = length(pointLights[i].position - position);
        float attenuation = 1.0 / (dist*dist);
        vec3 radiance = pointLights[i].color * attenuation;
        L0 += shade(N, V, L, albedo, roughness, metallic) * radiance;
    }

    L0 += shade(N, V, -sunLight.dir, albedo, roughness, metallic) * sunLight.color * shadow(position);

    // ambient lighting (env map)
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    // vec3 kD = vec3(1.) - max(fresnelShlick(dot(N, V), F0), 0);
    vec3 ambient = ssao * diffuse * (1. - metallic) * ambientIntensity;

    vec3 color = L0 + ambient;
    color = color / (color + vec3(1.));

    FragColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), opacity);
}
