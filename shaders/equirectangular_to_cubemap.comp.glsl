#version 430 core
layout(local_size_x = 8, local_size_y = 8) in;

uniform sampler2D envmap;
writeonly uniform image2D cubeface;
//uniform vec3 facedir;
//uniform vec3 right;

void main() {
    //vec3 up = cross(right, facedir);
    uvec2 img_size = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;
    vec2 face_uv = vec2(gl_GlobalInvocationID.xy) / vec2(img_size);
    vec4 env_color = texture(envmap, face_uv);
    imageStore(cubeface, ivec2(gl_GlobalInvocationID.xy), env_color);
}
