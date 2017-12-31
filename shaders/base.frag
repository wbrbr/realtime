#version 330 core

// in vec2 texcoords;
out vec4 outColor;

// uniform sampler2D tex;

void main()
{
    // outColor = texture(tex, texcoords);
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}