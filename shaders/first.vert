#version 460

layout(set = 2, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec3 v_normal; 
layout(location = 2) out vec3 v_frag_world;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);

    vec4 world_pos = ubo.model * vec4(in_position, 1.0);
    v_frag_world = world_pos.xyz;

    v_normal = in_normal;
    v_uv = in_uv;
}