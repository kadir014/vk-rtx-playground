#ifndef LAVA_MATH_H
#define LAVA_MATH_H

#include <math.h>


#define LV_PI         (float)( 3.141592653589793238462643383279502884)
#define LV_DEG_TO_RAD (float)( 0.017453292519943295769236907684886127)
#define LV_RAD_TO_DEG (float)(57.295779513082320876798154814105170336)

#define LV_RADIANS(d) ((d) * LV_DEG_TO_RAD)
#define LV_DEGREES(r) ((r) * LV_RAD_TO_DEG)


/**
 * @brief Clamp value between a range.
 * 
 * @param value Initial value.
 * @param min Minimum value.
 * @param max Maximum value.
 * @return Clamped value in range [min, max].
 */
static inline float lv_clamp(float value, float min, float max) {
    // https://stackoverflow.com/a/16659263
    const float t = value < min ? min : value;
    return t > max ? max : t;
}


/* For possible future reimplementations. */

#define lv_sqrtf sqrtf
#define lv_sinf sinf
#define lv_cosf cosf
#define lv_tanf tanf


#endif // LAVA_MATH_H