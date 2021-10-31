#version 430 core
#define M_PI 3.1415
layout(local_size_x = 8, local_size_y = 8) in;

uniform sampler2D envmap;
writeonly uniform imageCube cubeface;
uniform vec3 face_center;
uniform vec3 right;
uniform vec3 up;
uniform int layer;
//uniform vec3 facedir;
//uniform vec3 right;

void main() {
    uvec2 img_size = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;
    vec2 face_uv = vec2(gl_GlobalInvocationID.xy) / vec2(img_size) * 2 - 1;
    vec3 dir = normalize(face_center + face_uv.x * right + face_uv.y * up);

    //vec2 env_uv = vec2(asin(dir.y) / M_PI + 0.5, atan(dir.z, dir.x) / (2*M_PI) + 0.5);
    vec2 env_uv = vec2(atan(dir.z, dir.x) / (2*M_PI) + 0.5, asin(dir.y) / M_PI + 0.5);
    //imageStore(cubeface, ivec3(gl_GlobalInvocationID.xy, layer), vec4(env_uv, 0, 1));
    vec4 env_color = texture(envmap, env_uv);
    imageStore(cubeface, ivec3(gl_GlobalInvocationID.xy, layer), env_color);
}
