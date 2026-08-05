#version 460

layout(location = 0) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec4 f_color;

layout(set = 1, binding = 0) uniform sampler2D s_texture;

void main() {
    vec2 uv = vec2(v_uv.x, 1.0 - v_uv.y);

    // [-1, 1] -> [0, 1]
    vec3 r_normal = v_normal * 0.5 + 0.5;

    vec3 color = texture(s_texture, uv).rgb;

    f_color = vec4(color, 1.0);
}