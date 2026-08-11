#include "lava/world/camera.h"
#include "lava/math/math.h"
#include "lava/math/vector.h"


lvCamera lvCamera_new_perspective(
    float aspect_ratio,
    float near_z,
    float far_z,
    float fov
) {
    lvCamera camera = {0};

    camera.projection = lvCameraProjection_PERSPECTIVE;
    camera.mode = lvCameraMode_FIRST_PERSON;

    // TODO: aspect ratio yerine width height alırsan perspective-ortho kolay olur tek fonksiyonda?
    camera.proj_mat = lvMatrix4_perspective(
        LV_RADIANS(fov),
        aspect_ratio,
        near_z,
        far_z,
        -1.0f
    );

    camera.position = lvVector3_zero;
    camera.front = lv_vector3(-1.0f, 0.0f, 0.0f);
    camera.up = lv_vector3(0.0f, 1.0f, 0.0f);

    camera.yaw = -90.0f;
    camera.pitch = 0.0f;

    camera.target = lvVector3_zero;
    camera.distance = 0.0f;

    return camera;
}

void lvCamera_update(lvCamera *camera) {
    const float pitch_error = 0.1f;
    camera->pitch = lv_clamp(camera->pitch, -90.0f + pitch_error, 90.0f - pitch_error);

    float pitch_r = LV_RADIANS(camera->pitch);
    float yaw_r = LV_RADIANS(camera->yaw);
    float pitch_c = lv_cosf(pitch_r);
    float pitch_s = lv_sinf(pitch_r);
    float yaw_c = lv_cosf(yaw_r);
    float yaw_s = lv_sinf(yaw_r);

    // spherical -> cartesian
    camera->front = lv_vector3(
        yaw_c * pitch_c,
        pitch_s,
        yaw_s * pitch_c
    );
    camera->front = lvVector3_normalize(camera->front);

    switch (camera->mode) {
        case lvCameraMode_FIRST_PERSON:
            camera->target = lvVector3_add(camera->position, camera->front);
            break;
        
            case lvCameraMode_ORBIT:
            // TODO Orbit mode
            break;
    }

    camera->view_mat = lvMatrix4_look_at(camera->position, camera->target, camera->up);
}

void lvCamera_move(lvCamera *camera, float amount) {
    camera->position = lvVector3_add(camera->position, lvVector3_mul(camera->front, amount));
}

void lvCamera_strafe(lvCamera *camera, float amount) {
    lvVector3 right = lvVector3_normalize(lvVector3_cross(camera->front, camera->up));
    camera->position = lvVector3_add(camera->position, lvVector3_mul(right, amount));
}