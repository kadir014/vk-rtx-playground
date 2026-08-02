#ifndef LAVA_INTERNAL_ALLOC_H
#define LAVA_INTERNAL_ALLOC_H

#include "stdlib.h"
#include "stdint.h"


/**
 * @brief Report current memory leaks.
 * 
 * Call this function at the end of your application to get a brief report
 * on the tracked memory allocations so far.
 */
void lv_check_leaks();


void *_lv_malloc(size_t size, const char *file, uint32_t line);

void *_lv_realloc(void *ptr, size_t new_size, const char *file, uint32_t line);

void _lv_free(void *ptr, const char *file, uint32_t line);


#endif // LAVA_INTERNAL_ALLOC_H