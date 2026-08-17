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

    if (growth_factor <= 1.0f || default_capacity == 0 || elem_size == 0) {
        LV_THROW(
            "One (or all) of the arguments is invalid:\n"
            "  growth_factor (%f) must be higher than 1.0.\n"
            "  default_capacity (%zu) must be higher than 0.\n"
            "  elem_size (%zu) must be higher than 0.\n",
            growth_factor, default_capacity, elem_size
        );
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

lv_bool lvArray_valid(const lvArray *array) {
    return !(
        !array ||
        !array->data ||
        array->growth_factor <= 1.0f ||
        array->capacity == 0 ||
        array->size > array->capacity ||
        array->element_size == 0
    );
}

lvResult lvArray_add(lvArray *array, void *elem) {
    if (!array || !elem) {
        LV_THROW_AND_RETURN(
            lvResult_INVALID_ARGUMENTS,
            "One (or all) of the arguments is NULL:\n"
            "  array: %p\n"
            "  elem:  %p\n",
            array, elem
        );
    }

    // Only reallocate when max capacity is reached
    if (array->size == array->capacity) {
        size_t new_capacity = (size_t)((float)array->capacity * array->growth_factor);

        void *new_data = LV_REALLOC(
            array->data,
            new_capacity * array->element_size
        );

        if (!new_data) {
            LV_THROW_EMPTY_AND_RETURN(lvResult_FAILED_TO_ALLOCATE);
        }

        array->capacity = new_capacity;
        array->data = new_data;
    }

    memcpy(
        (char *)array->data + (array->size++) * array->element_size,
        elem,
        array->element_size
    );

    return lvResult_OK;
}

lvResult lvArray_resize(lvArray *array) {
    if (!array) {
        LV_THROW_AND_RETURN(
            lvResult_INVALID_ARGUMENTS,
            "One (or all) of the arguments is NULL:\n"
            "  array: %p\n",
            array
        );
    }

    if (array->size == array->capacity) {
        return lvResult_OK;
    }

    size_t new_capacity = array->size;

    void *new_data = LV_REALLOC(
        array->data,
        new_capacity * array->element_size
    );

    if (!new_data) {
        return lvResult_FAILED_TO_ALLOCATE;
    }

    array->capacity = new_capacity;
    array->data = new_data;

    return lvResult_OK;
}