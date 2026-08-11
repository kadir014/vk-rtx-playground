#ifndef LAVA_VK_RESOURCE_H
#define LAVA_VK_RESOURCE_H

#include "lava/internal.h"
#include "lava/vk/context.h"


/**
 * @brief Resource scope definition.
 * 
 * Defines which descriptor set the resource is bound to internally according
 * to its update frequency, object lifetime and usage domain.
 * 
 * The scope also determines how many descriptor set copies are created.
 * 
 * To determine which scope you need to use, refer to explanation in each enum field.
 */
typedef enum {
    lvResourceScope_GLOBAL, /**<
        Per-frame scope resource (only `frame_lag` copies).

        Useful for shared global resources that may be rewritten every frame, such as
        camera projection, animation delta time, screen resolution, lighting parameters, etc.

        Internally assigned to 0th set, use `layout(set=0, ...)` in your shader code.
    */

    lvResourceScope_MATERIAL, /**<
        Per-material scope resource (as many copies as there are materials).

        Useful for static, never (or very rarely) changed resources like material textures
        (albedo, normal maps, etc.) and other material-specific parameters.

        Internally assigned to 1st set, use `layout(set=1, ...)` in your shader code.
    */

    lvResourceScope_OBJECT, /**<
        Both per-model and per-frame scope resource (`frame_lag * amount_of_models` copies).

        Useful for object-specific resources that may be rewritten every frame, such as
        model matrix, object color, skeletal animation information, etc.

        Internally assigned to 2nd set, use `layout(set=2, ...)` in your shader code.
    */

    lvResourceScope_STATIC, /**<
        Application-wide scope resource (no copies).

        Useful for resources that are set once, and never (or very rarely) updated,
        such as look up tables, environment maps, precomputed noise textures, etc.

        @note This resource type is not strictly immutable, but since there is only 1 copy,
        mutating the resource while being used by the GPU is a synchronization hazard.

        Internally assigned to 3rd set, use `layout(set=3, ...)` in your shader code.
    */

    // TODO: lvResourceScope_INSTANCE - `frame_lag * n_instances` copies
} lvResourceScope;

typedef enum {
    lvResourceType_UNIFORM,
    lvResourceType_SAMPLER,
    lvResourceType_ACCELERATION_STRUCTURE
} lvResourceType;


#endif // LAVA_VK_RESOURCE_H