#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

#define NUM_SAMPLES 64

layout(binding = 0) uniform sampler2D depthtex;
layout(binding = 1) uniform sampler2D normaltex;
layout(binding = 2) uniform sampler2D noisetex;

uniform vec3 samples[NUM_SAMPLES];

uniform mat4 worldtoview;
uniform mat4 projection; // view space to clip space

uniform float radius;
uniform float bias;

vec3 reconstruct_view_position(vec2 uv)
{
    mat4 ClipToView = inverse(projection);

    vec2 xy = 2.0 * uv - vec2(1.0);
    float z = texture(depthtex, uv).r * 2 - 1;

    vec4 clip = vec4(xy, z, 1);
    vec4 view = ClipToView * clip;
    view.xyz /= view.w;
    view.w = 1;

    return view.xyz;
}

void main()
{
    vec3 randomvec = texture(noisetex, TexCoords * vec2(500., 300.)).rgb;
    vec3 N = normalize(mat3(worldtoview) * texture(normaltex, TexCoords).rgb); // by casting mat4 to mat3 we get rid of the translation
    vec3 position = reconstruct_view_position(TexCoords);

    vec3 tangent = normalize(randomvec - N * dot(randomvec, N));
    vec3 bitangent = cross(N, tangent);
    mat3 TBN = mat3(tangent, bitangent, N); // tangent space to view space matrix


    float occlusion = 0.0;
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        vec3 sample_ = TBN * samples[i]; // transform to view space
        sample_ = position + sample_ * radius; // offset fragment position with this sample

        // we want the screen position of this sample
        vec4 coords = vec4(sample_, 1.);
        coords = projection * coords; // we project the sample to the screen (clip space)

        // if the sample is outside the view frustum, assume that it doesn't occlude
        if (all(lessThanEqual(abs(coords.xyz), vec3(coords.w)))) {
            coords.xyz /= coords.w; // perspective divide (-> normalized device coordinates)
            coords.xyz = coords.xyz * .5 + .5; // [-1, 1] -> [0, 1]

            // TODO: don't compute the position, just linearize the depth
            float sampleDepth = reconstruct_view_position(coords.xy).z;
            float originalDepth = position.z;
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(originalDepth - sampleDepth));
            occlusion += (sampleDepth >= sample_.z + bias ? 1.0 : 0.0) * rangeCheck;
        }
    }

    float ret = 1.0 - (occlusion / NUM_SAMPLES);
    FragColor = vec4(ret, ret, ret, 1.);
}
