#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D history;
uniform sampler2D current;
uniform sampler2D world_positions;

uniform mat4 history_clip_from_world;
//uniform vec2 pixel_size;

void main() 
{
    vec3 current_color = texture(current, TexCoords).rgb;
    vec3 p_world = texture(world_positions, TexCoords).xyz;

    vec4 history_clip = history_clip_from_world * vec4(p_world, 1);
    history_clip.xy /= history_clip.w;

    if (p_world == vec3(0) || abs(history_clip.x) > 1 || abs(history_clip.y) > 1) {
        FragColor = vec4(current_color, 1);
        //FragColor = vec4(1, 0, 0, 1);
        return ;
    }

    // neighborhood clamping

    vec2 history_uv = history_clip.xy / 2 + 0.5;
    vec3 history_color = texture(history, history_uv).rgb;

    // TODO: fix the NaNs instead of this
    if (any(isnan(history_color)) || any(isinf(history_color))) {
        FragColor = vec4(current_color, 1);
        return;
    }

    FragColor = vec4(0.9 * history_color + 0.1 * current_color, 1);
}
