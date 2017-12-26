#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 TexCoords;

out vec2 texcoords;

void main()
{
    texcoords = TexCoords;
    gl_Position = vec4(position, 1.0);
}