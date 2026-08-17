#ifndef LAVA_ARRAY_H
#define LAVA_ARRAY_H

#include "lava/internal.h"


/**
 * @brief Dynamically growing type-specific array.
 */
typedef struct {
    size_t size; /**< Current number of elements in the array. */
    size_t capacity; /**< Number of elements that can be stored without reallocating. (Basically size of the currently allocated space.) */
    size_t element_size; /**< Size of one element in bytes. */
    float growth_factor; /**< Capacity multiplier used during reallocations. Must be higher than 1. */
    void *data; /**< Value array. */
} lvArray;

/**
 * @brief Fetch element at given index.
 * 
 * @param array Pointer to array.
 * @param index Index of the element.
 * @param type Type of the element.
 * @return Element at given index. Note that no bounds check is done.
 */
#define LV_ARRAY_AT(array, index, type) (*(type *)((char *)(array)->data + (index) * (array)->element_size))

/**
 * @brief Get pointer of element at given index.
 * 
 * @param array Pointer to array.
 * @param index Index of the element.
 * @param type Type of the element.
 * @return Element at given index. Note that no bounds check is done.
 */
#define LV_ARRAY_PTR_AT(array, index, type) ((type *)((char *)(array)->data + (index) * (array)->element_size))

/**
 * @brief Create a new array.
 * 
 * Use @ref lvArray_valid to see if creation was successful. If failed, use
 * @ref lv_get_error to get more information.
 * 
 * @param elem_size Size of one element.
 * @return lvArray
 */
lvArray lvArray_new(size_t elem_size);

/**
 * @brief Create a new array.
 * 
 * Use @ref lvArray_valid to see if creation was successful. If failed, use
 * @ref lv_get_error to get more information.
 * 
 * @param elem_size Size of one element.
 * @param default_capacity Initial number of elements to allocate the array with.
 * @param growth_factor Capacity multiplier used during reallocations. Must be higher than 1.
 * @return lvArray 
 */
lvArray lvArray_new_ex(
    size_t elem_size,
    size_t default_capacity,
    float growth_factor
);

/**
 * @brief Destroy the array.
 * 
 * It's safe to pass `NULL` to this function.
 * 
 * @param array Array to destroy.
 */
void lvArray_free(lvArray *array);

/**
 * @brief Check if array is valid.
 * 
 * @param array Array.
 * @return Whether the state is valid or not.
 */
lv_bool lvArray_valid(const lvArray *array);

/**
 * @brief Append new element at the end of the array.
 * 
 * @param array Array.
 * @param elem Element to add.
 * @return Possible error codes are listed below, use @ref lv_get_error to get more information:
 * - lvResult_INVALID_ARGUMENTS
 * - lvResult_FAILED_TO_ALLOCATE
 */
lvResult lvArray_add(lvArray *array, void *elem);

/**
 * @brief Synchronize the reserved space with current size.
 * 
 * Use this function only if you manually updated the `size` member.
 * 
 * @param array Array.
 * @return Possible error codes are listed below, use @ref lv_get_error to get more information:
 * - `lvResult_INVALID_ARGUMENTS`
 * - `lvResult_FAILED_TO_ALLOCATE`
 */
lvResult lvArray_resize(lvArray *array);


#endif // LAVA_ARRAY_H