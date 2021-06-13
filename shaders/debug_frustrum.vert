#version 330 core

in vec3 position;

uniform mat4 clipToWorld;
uniform mat4 viewProj;

void main()
{
    vec4 v = clipToWorld * vec4(position, 1.);
    v /= v.w;
    // gl_Position = v;
    gl_Position = viewProj * v;
}