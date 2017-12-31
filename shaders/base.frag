#version 330 core

in vec3 Normal;

// in vec2 texcoords;
out vec4 outColor;

// uniform sampler2D tex;
uniform vec3 light;

void main()
{
    // outColor = texture(tex, texcoords);
    // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    // outColor = vec4(Normal, 1.0);
    float intensity = dot(Normal, normalize(light)); // TODO: calculate light-to-object instead of assuming (0, 0, 0)
    outColor = vec4(intensity, intensity, intensity, 1.f);
}