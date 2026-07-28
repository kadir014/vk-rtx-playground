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

    array.data = malloc(elem_size * default_capacity);

    return array;
}

void lvArray_free(lvArray *array) {
    if (!array) {
        return;
    }

    free(array->data);
}

bool lvArray_valid(lvArray *array) {
    return !(
        !array->data ||
        array->growth_factor <= 1.0f ||
        array->size > array->capacity
    );
}

int lvArray_add(lvArray *array, void *elem) {
    // Only reallocate when max capacity is reached
    if (array->size == array->capacity) {
        array->size++;

        size_t new_capacity = (size_t)((float)array->capacity * array->growth_factor);
        array->capacity = new_capacity;

        array->data = realloc(array->data, array->capacity * array->element_size);
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