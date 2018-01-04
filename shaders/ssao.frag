#version 330 core

in vec3 Position;
in vec3 Normal;

out vec4 outColor;

uniform vec3 light;
uniform sampler2D depth_texture;

void main()
{
    /* vec3 obj_to_light = normalize(light - Position);
    float intensity = dot(Normal, obj_to_light);
    vec4 color = vec4(intensity, intensity, intensity, 1.f); */
    vec2 texcoords;
    texcoords.x = gl_FragCoord.x / 800.0;
    texcoords.y = gl_FragCoord.y / 450.0;
    float depth = texture(depth_texture, texcoords).r;
    vec3 color = vec3(depth, depth, depth);
    outColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), 1.0);
}