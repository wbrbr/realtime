#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D tex;
uniform sampler2D depth;
uniform float zFar;
uniform float zNear;

const float WIDTH = 800.0;
const float HEIGHT = 450.0;

float linearizeDepth(float d)
{
    d = 2.0 * d - 1.0;
    return (2.0 * zNear)  / (zFar + zNear - d * (zFar - zNear));
}

float sobel()
{
    float xinc = 1.0 / WIDTH;
    float yinc = 1.0 / HEIGHT;

    float top = linearizeDepth(texture(depth, vec2(TexCoords.x, TexCoords.y + yinc)).r);
    float bottom = linearizeDepth(texture(depth, vec2(TexCoords.x, TexCoords.y - yinc)).r);
    float left = linearizeDepth(texture(depth, vec2(TexCoords.x - xinc, TexCoords.y)).r);
    float right = linearizeDepth(texture(depth, vec2(TexCoords.x + xinc, TexCoords.y)).r);
    float bottomLeft = linearizeDepth(texture(depth, vec2(TexCoords.x - xinc, TexCoords.y - yinc)).r);
    float bottomRight = linearizeDepth(texture(depth, vec2(TexCoords.x + xinc, TexCoords.y - yinc)).r);
    float topLeft = linearizeDepth(texture(depth, vec2(TexCoords.x - xinc, TexCoords.y + yinc)).r);
    float topRight = linearizeDepth(texture(depth, vec2(TexCoords.x + xinc, TexCoords.y + yinc)).r);
    float sx = -topLeft - 2.0 * left - bottomLeft + topRight   + 2.0 * right  + bottomRight;
    float sy = -topLeft - 2.0 * top  - topRight   + bottomLeft + 2.0 * bottom + bottomRight;
    return sqrt(sx*sx + sy*sy);
}

const float threshold = 0.3;

void main()
{
    vec3 color = texture(tex, TexCoords).rgb;
    color = color / (color + vec3(1.0));

    float depth = linearizeDepth(texture(depth, TexCoords).r);
    float s = sobel();
    if (s > threshold) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), 1.0);
    }
}