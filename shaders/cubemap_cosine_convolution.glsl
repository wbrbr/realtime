#version 430 core
#define NUM_SAMPLES 16
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform samplerCube cubemap;
writeonly uniform imageCube irradiance_map;
uniform vec3 vectors_with_cos_distribution[NUM_SAMPLES];

vec3 cubemap_uv_to_direction(vec2 uv, uint layer)
{
    switch(layer)
    {
    case 0: // +X
        return normalize(vec3(1, 1-2*uv.y, 1-2*uv.x));

    case 1: // -X
        return normalize(vec3(-1, 1-2*uv.y, 2*uv.x-1));

    case 2: // +Y
        return normalize(vec3(2*uv.x-1, 1, 2*uv.y-1));

    case 3: // -Y
        return normalize(vec3(2*uv.x-1, -1, 1-2*uv.y));

    case 4: // +Z
        return normalize(vec3(2*uv.x-1, 1-2*uv.y, 1));

    case 5: // -Z
        return normalize(vec3(1-2*uv.x, 1-2*uv.y, -1));

    default:
        return vec3(0,0,0);
    }
}

void main() {
    uvec2 img_size = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;
    vec2 face_uv = vec2(gl_GlobalInvocationID.xy) / vec2(img_size);
    uint layer = gl_GlobalInvocationID.z;

    vec3 n = cubemap_uv_to_direction(face_uv, layer);

    vec3 irradiance = vec3(0);

    vec3 b1 = cross(n, vec3(1, 0, 0));
    if (dot(b1, b1) < 0.01) {
        b1 = cross(n, vec3(0, 1, 0));
    }
    b1 = normalize(b1);
    vec3 b2 = cross(n, b1);

    //imageStore(irradiance_map, ivec3(gl_GlobalInvocationID), texture(cubemap, n));

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        vec3 v = vectors_with_cos_distribution[i];
        vec3 w_i = v.x * b1 + v.y * b2 + v.z * n;
        irradiance += texture(cubemap, w_i).rgb;
    }
    irradiance /= NUM_SAMPLES;

    imageStore(irradiance_map, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1));
}
