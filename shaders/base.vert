#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;

uniform mat4 model;
uniform mat4 viewproj;

out vec3 Position;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    // texcoords = TexCoords;
    Normal = normal;
    Position = (model * vec4(position, 1.0)).xyz;
    TexCoords = texcoords;
    gl_Position = viewproj * model * vec4(position, 1.0);
}