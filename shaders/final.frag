#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D tex;

void main()
{
    vec3 color = texture(tex, TexCoords).rgb;
    FragColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), 1.0);
}