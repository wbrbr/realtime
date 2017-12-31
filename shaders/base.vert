#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 mvp;

out vec3 Normal;

void main()
{
    // texcoords = TexCoords;
    Normal = normal;
    gl_Position = mvp * vec4(position, 1.0);
}