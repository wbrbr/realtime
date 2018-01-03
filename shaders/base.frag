#version 330 core

in vec3 Position;
in vec3 Normal;

out vec4 outColor;

uniform vec3 light;

void main()
{
    vec3 obj_to_light = normalize(light - Position);
    float intensity = dot(Normal, obj_to_light);
    outColor = vec4(intensity, intensity, intensity, 1.f);
}