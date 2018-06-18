#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    vec3 color = texture(skybox, TexCoords).rgb;
    FragColor = vec4(pow(color.r, 2.2), pow(color.g, 2.2), pow(color.b, 2.2), 1.0);
}