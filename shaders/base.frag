#version 330 core

in vec3 Position;
in vec3 Normal;

out vec4 outColor;

uniform vec3 light;

void main()
{
    vec3 obj_to_light = normalize(light - Position);
    float intensity = dot(Normal, obj_to_light);
    vec4 color = vec4(intensity, intensity, intensity, 1.f);
    outColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), 1.0);
}