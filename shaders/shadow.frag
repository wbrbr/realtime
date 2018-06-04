#version 330 core

in vec3 Position;
in vec3 Normal;
in vec2 TexCoords;

out vec4 outColor;

uniform vec3 light;
uniform vec3 camera_position;
uniform sampler2D depth_texture;
uniform samplerCube skybox;
uniform sampler2D image_texture;
uniform mat4 lightSpace;

void main()
{
    vec2 texcoords;
    texcoords.x = gl_FragCoord.x / 800.0;
    texcoords.y = gl_FragCoord.y / 450.0;
    float depth = texture(depth_texture, texcoords).r;
    vec4 fragPosLightSpace = lightSpace * vec4(Position, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(depth_texture, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float visibility = currentDepth - 0.05< closestDepth ? 1.0 : 0.0;
    // vec3 color = vec3(depth, depth, depth);
    vec3 obj_to_light = normalize(light - Position);
    float intensity = dot(Normal, obj_to_light) * visibility;

    vec3 incident = Position - camera_position;
    vec3 reflected = reflect(incident, Normal);

    // vec3 texColor = texture(image_texture, TexCoords).rgb;
    vec3 texColor = texture(skybox, reflected).rgb;

    vec4 color = vec4(texColor * intensity, 1.f);
    outColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), 1.0);
}