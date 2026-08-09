#ifndef LAVA_VK_RESOURCE_H
#define LAVA_VK_RESOURCE_H

#include "lava/internal.h"
#include "lava/vk/context.h"


/**
 * @brief Resource update frequency.
 * 
 * Defines which descriptor set the resource is bound to internally according
 * to its update frequency.
 */
typedef enum {
    lvResourceFreq_GLOBAL, /**< Per-frame scope resource (only `frame_lag` copies).
                                Useful for shared global resources that may be rewritten every frame, such as
                                camera projection, animation delta time, screen resolution, etc.
                                Internally assigned to 0th set, use `layout(set=0, ...)` in your shader code. */
    lvResourceFreq_MATERIAL, /**< Per-material scope resource (as many copies as there are materials).
                                  Useful for static, never (or very rarely) changed resources like material textures.
                                  Internally assigned to 1st set, use `layout(set=1, ...)` in your shader code. */
    lvResourceFreq_OBJECT, /**< Both per-model and per-frame scope resource (`frame_lag * amount_of_models` copies).
                                Useful for object-specific resources that may be rewritten every frame, such as
                                model matrix, skeletal animation information, etc.
                                Internally assigned to 2nd set, use `layout(set=2, ...)` in your shader code. */
    lvResourceFreq_STATIC, /**< Constant immutable scope resource (no copies).
                                Useful for resources that are set once, and never updated ever, such as
                                look up tables, environment maps, precomputed noise textures, etc. 
                                Internally assigned to 3rd set, use `layout(set=3, ...)` in your shader code. */
} lvResourceFreq;

typedef enum {
    lvResourceType_UNIFORM,
    lvResourceType_SAMPLER,
} lvResourceType;


#endif // LAVA_VK_RESOURCE_H