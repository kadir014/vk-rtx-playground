#version 460

//layout(location = 0) in vec4 v_color;
layout(location = 0) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec4 f_color;

layout(binding = 1) uniform sampler2D s_texture;

void main() {
    f_color = texture(s_texture, v_uv);
}