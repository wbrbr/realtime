#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D history;
uniform sampler2D current;
uniform sampler2D world_positions;

uniform mat4 history_clip_from_world;
uniform vec2 pixel_size;
uniform vec2 jitter_ndc;
uniform bool neighborhood_clamping;

void main() 
{
    vec3 current_color = texture(current, TexCoords + jitter_ndc / 2).rgb;
    vec3 p_world = texture(world_positions, TexCoords).xyz;

    vec4 history_clip = history_clip_from_world * vec4(p_world, 1);
    history_clip.xy /= history_clip.w;

    if (abs(history_clip.x) > 1 || abs(history_clip.y) > 1) {
        FragColor = vec4(current_color, 1);
        return ;
    }


    vec3 min_rgb = texture(current, TexCoords + jitter_ndc / 2).rgb;
    vec3 max_rgb = min_rgb;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            vec2 uv = TexCoords + vec2(i,j) * pixel_size;
            //uv += jitter_ndc / 2;

            vec3 c = texture(current, uv).rgb;
            min_rgb = min(min_rgb, c);
            max_rgb = max(max_rgb, c);
        }
    }

    vec2 history_uv = history_clip.xy / 2 + 0.5;
    if (p_world == vec3(0)) {
        history_uv = TexCoords + jitter_ndc / 2;
    }
    vec3 history_color = texture(history, history_uv).rgb;

    // neighborhood clamping
    if (neighborhood_clamping) {
        history_color = clamp(history_color, min_rgb, max_rgb);
    }

    // TODO: fix the NaNs instead of this
    if (any(isnan(history_color)) || any(isinf(history_color))) {
        FragColor = vec4(current_color, 1);
        return;
    }

    FragColor = vec4(0.9 * history_color + 0.1 * current_color, 1);
}
