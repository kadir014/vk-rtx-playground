#ifndef LAVA_INTERNAL_H
#define LAVA_INTERNAL_H


#include <stdlib.h>
#include <stdbool.h>


#define LV_INVALID_INDEX_ZU (size_t)(-1)


#ifndef LV_MALLOC
    #define LV_MALLOC(size) malloc(size)
#endif

#ifndef LV_REALLOC
    #define LV_REALLOC(ptr, new_size) realloc(ptr, new_size)
#endif

#ifndef LV_FREE
    #define LV_FREE(ptr) free((void *)(ptr))
#endif


#endif // LAVA_INTERNAL_H