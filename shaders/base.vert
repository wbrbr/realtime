#version 330 core
layout(location = 0) in vec3 position;
// layout(location = 1) in vec2 TexCoords;

uniform mat4 mvp;
// out vec2 texcoords;

void main()
{
    // texcoords = TexCoords;
    gl_Position = mvp * vec4(position, 1.0);
}