#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D tex;
uniform float zNear;
uniform float zFar;

float linearDepth(float d)
{
    return 2.0*zNear / (zFar + zNear - (zFar - zNear)*(2*d -1));
}

void main() 
{
    FragColor = vec4(vec3(linearDepth(texture(tex, TexCoords).r)), 1.);
}