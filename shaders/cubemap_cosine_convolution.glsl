#version 430 core
#define NUM_SAMPLES 128
#define M_PI 3.1415
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform samplerCube cubemap;
uniform sampler3D noise_tex;
writeonly uniform imageCube irradiance_map;

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

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        vec2 rnd = texelFetch(noise_tex, ivec3(gl_GlobalInvocationID.xy, i), 0).xy;
        float r = sqrt(rnd.x);
        float theta = 2.0 * M_PI * rnd.y;
        float x = r * cos(theta);
        float y = r * sin(theta);
        float z = sqrt(max(0, 1 - x*x - y*y));

        vec3 w_i = x * b1 + y * b2 + z * n;
        irradiance += texture(cubemap, w_i).rgb;
    }
    irradiance /= NUM_SAMPLES;

    imageStore(irradiance_map, ivec3(gl_GlobalInvocationID), vec4(irradiance, 1));
}
