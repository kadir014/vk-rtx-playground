#version 460

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec4 f_color;

void main() {
    f_color = vec4(v_uv, 0.0, v_color.a);
}