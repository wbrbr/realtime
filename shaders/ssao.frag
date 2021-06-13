#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D positiontex;
uniform sampler2D normaltex;
uniform sampler2D roughmettex;
uniform sampler2D noisetex;

uniform vec3 samples[64];

uniform mat4 worldtoview;
uniform mat4 projection; // view space to clip space

void main()
{
    vec3 randomvec = texture(noisetex, TexCoords * vec2(500., 300.)).rgb;
    vec3 N = normalize(mat3(worldtoview) * texture(normaltex, TexCoords).rgb); // by casting mat4 to mat3 we get rid of the translation
    vec3 position = vec3(worldtoview * texture(positiontex, TexCoords).rgba);

    vec3 tangent = normalize(randomvec - N * dot(randomvec, N));
    vec3 bitangent = cross(N, tangent);
    mat3 TBN = mat3(tangent, bitangent, N); // tangent space to view space matrix


    float occlusion = 0.0;
    const float radius = .03;
    const float bias = .025;
    for (int i = 0; i < 64; i++)
    {
        vec3 sample = TBN * samples[i]; // transform to view space
        sample = position + sample * radius; // offset fragment position with this sample

        // we want the screen position of this sample
        vec4 coords = vec4(sample, 1.);
        coords = projection * coords; // we project the sample to the screen (clip space)
        coords.xyz /= coords.w; // perspective divide (-> normalized device coordinates)
        coords.xyz = coords.xyz * .5 + .5; // [-1, 1] -> [0, 1]
        float sampleDepth = (worldtoview * texture(positiontex, coords.xy)).z;
        float opaque = texture(roughmettex, coords.xy).z;
        occlusion += (sampleDepth >= sample.z ? 1.0 : 0.0);
        float originalDepth = (worldtoview * texture(positiontex, TexCoords)).z;
        float rangeCheck = opaque > 0.1 ? smoothstep(0.0, 1.0, radius / abs(originalDepth - sampleDepth)) : 0.0;
        occlusion += (sampleDepth >= sample.z ? 1.0 : 0.0) * rangeCheck;   
    }

    occlusion = 1.0 - (occlusion / 64);
    FragColor = vec4(occlusion, occlusion, occlusion, 1.);
}
