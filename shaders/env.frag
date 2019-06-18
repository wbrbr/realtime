#version 330 core
out vec4 FragColor;
in vec3 TexCoords;
uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(TexCoords));
    vec3 color = texture(equirectangularMap, uv).rgb;
    color = color / (color + vec3(1.0));
    FragColor = vec4(pow(color.r, 0.4545), pow(color.g, 0.4545), pow(color.b, 0.4545), 1.0);
}
