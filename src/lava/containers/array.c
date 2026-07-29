#include <string.h>
#include "lava/containers/array.h"


lvArray lvArray_new(size_t elem_size) {
    return lvArray_new_ex(elem_size, 1, 2.0f);
}

lvArray lvArray_new_ex(
    size_t elem_size,
    size_t default_capacity,
    float growth_factor
) {
    lvArray array = {
        .size = 0,
        .capacity = default_capacity,
        .element_size = elem_size,
        .growth_factor = growth_factor,
        .data = NULL
    };

    if (growth_factor <= 1.0f) {
        return array;
    }

    array.data = LV_MALLOC(elem_size * default_capacity);

    return array;
}

void lvArray_free(lvArray *array) {
    if (!array) {
        return;
    }

    LV_FREE(array->data);
}

bool lvArray_valid(lvArray *array) {
    return !(
        !array ||
        !array->data ||
        array->growth_factor <= 1.0f ||
        array->size > array->capacity
    );
}

int lvArray_add(lvArray *array, void *elem) {
    if (!array) {
        return 2;
    }

    // Only reallocate when max capacity is reached
    if (array->size == array->capacity) {
        array->size++;

        size_t new_capacity = (size_t)((float)array->capacity * array->growth_factor);
        array->capacity = new_capacity;

        array->data = LV_REALLOC(array->data, array->capacity * array->element_size);
        if (!array->data) {
            return 1;
        }
    }
    else {
        array->size++;
    }

    memcpy(
        (char *)array->data + (array->size - 1) * array->element_size,
        elem,
        array->element_size
    );

    return 0;
}

int lvArray_resize(lvArray *array) {
    if (!array) {
        return 2;
    }

    if (array->size == array->capacity) {
        return 0;
    }

    size_t new_capacity = array->size;

    void *new_data = realloc(
        array->data,
        new_capacity * array->element_size
    );

    if (!new_data) {
        return 1;
    }

    array->capacity = new_capacity;
    array->data = new_data;

    return 0;
}