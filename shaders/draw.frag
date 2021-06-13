#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D tex;
uniform vec2 jitter;

void main() 
{
    FragColor = texture(tex, TexCoords + jitter/2);
}
