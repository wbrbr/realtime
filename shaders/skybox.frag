#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
    vec3 col = texture(skybox, TexCoords).xyz;
    col = col / (vec3(.5) + col);
    FragColor = vec4(col, 1.);
}