#ifndef LAVA_INTERNAL_H
#define LAVA_INTERNAL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "lava/internal_alloc.h"


#define LV_INVALID_INDEX_ZU (size_t)(-1)
#define LV_INVALID_INDEX_U64 UINT64_MAX
#define LV_INVALID_INDEX_U32 UINT32_MAX


#ifndef LV_MALLOC
    #ifdef LV_DEBUG
        #define LV_MALLOC(size) _lv_malloc(size, __FILE__, __LINE__)
    #else
        #define LV_MALLOC(size) malloc(size)
    #endif
#endif

#ifndef LV_REALLOC
    #ifdef LV_DEBUG
        #define LV_REALLOC(ptr, new_size) _lv_realloc(ptr, new_size, __FILE__, __LINE__)
    #else
        #define LV_REALLOC(ptr, new_size) realloc(ptr, new_size)
    #endif
#endif

#ifndef LV_FREE
    #ifdef LV_DEBUG
        #define LV_FREE(ptr) _lv_free((void *)(ptr), __FILE__, __LINE__)
    #else
        #define LV_FREE(ptr) free((void *)(ptr))
    #endif
#endif


#endif // LAVA_INTERNAL_H