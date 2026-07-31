#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_uv;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 0.0, 1.0);
    v_color = in_color;
    v_uv = in_uv;
}