#version 330 core
layout(location = 0) in vec3 position;

out vec4 color;

void main()
{
    color.r = position.x;
    color.g = position.y;
    color.b = 0.0;
    color.a = 1.0;
    gl_Position = vec4(position, 1.0);
}