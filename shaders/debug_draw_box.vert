#version 330 core

in vec3 position;

uniform mat4 model;
uniform mat4 viewproj;

void main()
{
    gl_Position = viewproj * model * vec4(position, 1);
}