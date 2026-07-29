#include <string.h>
#include "lava/containers/refarray.h"


lvRefArray lvRefArray_new() {
    return lvRefArray_new_ex(1, 2.0f);
}

lvRefArray lvRefArray_new_ex(size_t default_capacity, float growth_factor) {
    lvRefArray refarray = {
        .size = 0,
        .capacity = default_capacity,
        .growth_factor = growth_factor,
        .data = NULL
    };

    if (growth_factor <= 1.0f) {
        return refarray;
    }

    refarray.data = LV_MALLOC(sizeof(void *) * default_capacity);

    /*
        The allocated data is not zeroed out for performance reasons.
        The usable space in the array is set whenever add function is used, rest
        must be assumed not usable.
    */

    return refarray;
}

void lvRefArray_free(lvRefArray *refarray) {
    if (!refarray) {
        return;
    }

    LV_FREE(refarray->data);
}

bool lvRefArray_valid(lvRefArray *refarray) {
    return !(
        !refarray->data ||
        refarray->growth_factor <= 1.0f ||
        refarray->size > refarray->capacity
    );
}

int lvRefArray_add(lvRefArray *refarray, void *elem) {
    // Only reallocate when max capacity is reached
    if (refarray->size == refarray->capacity) {
        refarray->size++;

        size_t new_capacity = (size_t)((float)refarray->capacity * refarray->growth_factor);
        refarray->capacity = new_capacity;

        refarray->data = LV_REALLOC(refarray->data, refarray->capacity * sizeof(void *));
        if (!refarray->data) {
            return 1;
        }
    }
    else {
        refarray->size++;
    }

    refarray->data[refarray->size - 1] = elem;

    return 0;
}

void *lvRefArray_pop(lvRefArray *refarray, size_t index) {
    if (refarray->size == 0 || index >= refarray->size) {
        return NULL;
    }

    void *elem = refarray->data[index];

    // Shift everything after index left by one position.
    if (index < refarray->size - 1) {
        memmove(
            &refarray->data[index],
            &refarray->data[index + 1],
            (refarray->size - index - 1) * sizeof(void *)
        );
    }

    refarray->size--;
    refarray->data[refarray->size] = NULL;

    return elem;
}

size_t lvRefArray_remove(lvRefArray *refarray, void *elem) {
    size_t index = LV_INVALID_INDEX_ZU;
    for (size_t i = 0; i < refarray->size; i++) {
        if (refarray->data[i] == elem) {
            index = i;
            break;
        }
    }

    if (index == LV_INVALID_INDEX_ZU) {
        return index;
    }

    if (!lvRefArray_pop(refarray, index)) {
        return LV_INVALID_INDEX_ZU;
    }
    else {
        return index;
    }
}

void lvRefArray_clear(lvRefArray *refarray, void (free_func)(void *)) {
    if (refarray->size == 0) {
        return;
    }

    for (size_t i = 0; i < refarray->size; i++) {
        if (free_func && refarray->data[i]) {
            free_func(refarray->data[i]);
        }
    }

    refarray->size = 0;
}

void lvRefArray_for_each(
    lvRefArray *refarray,
    nvRefArray_for_each_callback callback,
    void *user_data
) {
    for (size_t i = 0; i < refarray->size; i++) {
        callback(refarray->data[i], user_data);
    }
}

lvRefArray lvRefArray_copy(lvRefArray *refarray) {
    lvRefArray copy = lvRefArray_new_ex(refarray->capacity, refarray->growth_factor);
    if (!lvRefArray_valid(&copy)) return copy;

    copy.size = refarray->size;
    for (size_t i = 0; i < refarray->size; i++) {
        copy.data[i] = refarray->data[i];
    }

    return copy;
}