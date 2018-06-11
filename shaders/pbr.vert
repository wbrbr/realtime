#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoords;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 model;
uniform mat4 viewproj;
uniform vec3 lightPos;
uniform vec3 camPos;

out vec3 Position;
out vec2 TexCoords;
out mat3 TBN;

void main()
{
    vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 B = normalize(vec3(model * vec4(bitangent, 0.0)));
    TBN = mat3(T, B, N);
    // Normal = normal;
    Position = (model * vec4(position, 1.0)).xyz;
    TexCoords = texcoords;
    gl_Position = viewproj * model * vec4(position, 1.0);
}