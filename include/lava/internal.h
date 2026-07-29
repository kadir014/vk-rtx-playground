#ifndef LAVA_INTERNAL_H
#define LAVA_INTERNAL_H


#include <stdlib.h>
#include <stdbool.h>


#define LV_INVALID_INDEX_ZU (size_t)(-1)


#define LV_MALLOC(size) malloc(size)
#define LV_REALLOC(ptr, new_size) realloc(ptr, new_size)
#define LV_FREE(ptr) free((void *)ptr)


#endif // LAVA_INTERNAL_H