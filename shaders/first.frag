#version 460
#extension GL_EXT_ray_query: require

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_frag_world;

layout(location = 0) out vec4 f_color;

layout(set = 1, binding = 0) uniform sampler2D s_texture;

layout(set = 0, binding = 0) uniform accelerationStructureEXT u_tlas;


bool in_shadow(vec3 p) {
    bool hit = false;

    // Build the shadow ray from the world position toward the light

    vec3 light_dir = normalize(vec3(1.0, 3.0, 0.5));

    rayQueryEXT query;

    rayQueryInitializeEXT(
        query,
        u_tlas,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        p,
        0.0005, // tmin
        light_dir,
        1000.0 // tmax
    );

    rayQueryProceedEXT(query);

    uint intersection = rayQueryGetIntersectionTypeEXT(query, true);
    hit = intersection == gl_RayQueryCommittedIntersectionTriangleEXT;

    return hit;
}


void main() {
    vec2 uv = vec2(v_uv.x, 1.0 - v_uv.y);

    bool shadowed = in_shadow(v_frag_world);

    // [-1, 1] -> [0, 1]
    vec3 r_normal = v_normal * 0.5 + 0.5;

    vec3 color = texture(s_texture, uv).rgb;

    if (color.r == 0.0 && color.g == 1.0 && color.b == 1.0)
        f_color = vec4(r_normal, 1.0);
    else
        f_color = vec4(color, 1.0);

    if (shadowed) {
        f_color.rgb *= 0.15;
    }
}