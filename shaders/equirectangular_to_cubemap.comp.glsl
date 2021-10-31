#version 430 core
#define M_PI 3.1415
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

uniform sampler2D envmap;
writeonly uniform imageCube cubeface;

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

    vec3 dir = cubemap_uv_to_direction(face_uv, layer);
    dir.y *= -1; // opengl cubemap coordinates are weird

    vec2 env_uv = vec2(atan(dir.z, dir.x) / (2*M_PI) + 0.5, asin(dir.y) / M_PI + 0.5);
    vec4 env_color = texture(envmap, env_uv);
    imageStore(cubeface, ivec3(gl_GlobalInvocationID.xy, layer), env_color);
}
