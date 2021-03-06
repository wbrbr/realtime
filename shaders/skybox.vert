#version 330 core
layout (location = 0) in vec3 pos;
out vec3 TexCoords;

uniform mat4 viewproj;

void main()
{
    TexCoords = vec3(pos.x, pos.y, pos.z);
    gl_Position = viewproj * vec4(pos, 1.0);
}