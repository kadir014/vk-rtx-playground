#include "lava/containers/hashmap.h"


lvHashMap lvHashMap_new(
    size_t item_size,
    lvHashMap_hasher hasher,
    lvHashMap_comparer comparer
) {
    return lvHashMap_new_ex(item_size, hasher, comparer, 64, 2.0f, 0.75f);
}

lvHashMap lvHashMap_new_ex(
    size_t item_size,
    lvHashMap_hasher hasher,
    lvHashMap_comparer comparer,
    size_t default_capacity,
    float growth_factor,
    float growth_rule
) {
    lvHashMap hashmap = {
        .size = 0,
        .capacity = default_capacity,
        .item_size = item_size,
        .growth_factor = growth_factor,
        .growth_rule = growth_rule,
        .data = NULL,
        .data_state = NULL,
        .hasher = hasher,
        .comparer = comparer
    };

    if (
        growth_factor <= 1.0f ||
        growth_rule <= 0.0f ||
        growth_rule > 1.0f ||
        default_capacity == 0 ||
        item_size == 0
    ) {
        return hashmap;
    }

    hashmap.data = LV_MALLOC(item_size * default_capacity);
    if (!hashmap.data) {
        return hashmap;
    }

    hashmap.data_state = LV_CALLOC(default_capacity, sizeof(lv_bool));
    if (!hashmap.data_state) {
        LV_FREE(hashmap.data);
        return hashmap;
    }

    return hashmap;
}

void lvHashMap_free(lvHashMap *hashmap) {
    if (!hashmap) {
        return;
    }

    LV_FREE(hashmap->data_state);
    LV_FREE(hashmap->data);
}

lv_bool lvHashMap_valid(const lvHashMap *hashmap) {
    return !(
        !hashmap ||
        !hashmap->data ||
        !hashmap->data_state ||
        hashmap->growth_factor <= 1.0f ||
        hashmap->growth_rule <= 0.0f || hashmap->growth_rule > 1.0f ||
        hashmap->capacity == 0 ||
        hashmap->size > hashmap->capacity ||
        hashmap->item_size == 0
    );
}

void lvHashMap_clear(lvHashMap *hashmap) {
    hashmap->size = 0;
    memset(hashmap->data, 0, hashmap->item_size * hashmap->capacity);
    memset(hashmap->data_state, LV_FALSE, sizeof(lv_bool) * hashmap->capacity);
}

void *lvHashMap_get(const lvHashMap *hashmap, void *item) {
    if (!hashmap || !item) {
        LV_THROW_AND_RETURN(
            NULL,
            "One (or all) of the arguments is NULL:\n"
            "  hashmap: %p\n"
            "  item:    %p\n",
            hashmap, item
        );
    }

    lv_u64 hash = hashmap->hasher(item);
    lv_u64 hash_mod = hash % hashmap->capacity;

    // Linear probing
    while (hashmap->data_state[hash_mod] == LV_TRUE) {
        void *existing_item = (char *)hashmap->data + hash_mod * hashmap->item_size;

        if (hashmap->hasher(existing_item) == hash) {
            if (hashmap->comparer(existing_item, item)) {
                return existing_item;
            }
        }

        hash_mod = (hash_mod + 1) % hashmap->capacity;
    }

    return NULL;
}

static lv_bool resize_and_rehash(lvHashMap *hashmap, size_t new_capacity) {
    void *new_data = LV_MALLOC(new_capacity * hashmap->item_size);
    if (!new_data) {
        return LV_FALSE;
    }

    lv_bool *new_data_state = LV_CALLOC(new_capacity, sizeof(lv_bool));
    if (!new_data_state) {
        LV_FREE(new_data);
        return LV_FALSE;
    }

    // Re-hash and reinsert every item back into the reallocated space.
    for (size_t i = 0; i < hashmap->capacity; i++) {
        if (!hashmap->data_state[i]) {
            continue;
        }

        void *item = (char *)hashmap->data + i * hashmap->item_size;

        lv_u64 hash = hashmap->hasher(item);
        size_t index = hash % new_capacity;

        while (new_data_state[index]) {
            index = (index + 1) % new_capacity;
        }

        new_data_state[index] = LV_TRUE;

        memcpy(
            (char *)new_data + index * hashmap->item_size,
            item,
            hashmap->item_size
        );
    }

    LV_FREE(hashmap->data);
    LV_FREE(hashmap->data_state);

    hashmap->data = new_data;
    hashmap->data_state = new_data_state;
    hashmap->capacity = new_capacity;

    return LV_TRUE;
}

void *lvHashMap_set(lvHashMap *hashmap, void *item) {
    if (!hashmap || !item) {
        LV_THROW_AND_RETURN_NULL(
            "One (or all) of the arguments is NULL:\n"
            "  hashmap: %p\n"
            "  item:    %p\n",
            hashmap, item
        );
    }

    if ((float)hashmap->size >= (float)hashmap->capacity * hashmap->growth_rule) {
        size_t new_capacity = (size_t)((float)hashmap->capacity * hashmap->growth_factor);

        if (!resize_and_rehash(hashmap, new_capacity)) {
            LV_THROW_EMPTY_AND_RETURN_NULL();
        }
    }

    lv_u64 hash = hashmap->hasher(item);
    lv_u64 hash_mod = hash % hashmap->capacity;

    // Linear probing
    while (hashmap->data_state[hash_mod] == LV_TRUE) {
        void *existing_item = (char *)hashmap->data + hash_mod * hashmap->item_size;

        // If item already exists, update it
        if (hashmap->hasher(existing_item) == hash) {
            if (hashmap->comparer(existing_item, item)) {
                memcpy(existing_item, item, hashmap->item_size);

                return existing_item;
            }
        }

        // If not, keep searching
        hash_mod = (hash_mod + 1) % hashmap->capacity;
    }

    // Item doesn't exist in the hashmap, insert into first found empty slot
    hashmap->data_state[hash_mod] = LV_TRUE;
    void *new_item = (char *)hashmap->data + hash_mod * hashmap->item_size;
    memcpy(new_item, item, hashmap->item_size);

    hashmap->size++;

    return new_item;
}

lv_bool lvHashMap_contains(const lvHashMap *hashmap, void *item) {
    if (!hashmap || !item) {
        return LV_FALSE;
    }

    void *lookup = lvHashMap_get(hashmap, item);
    return lookup != NULL;
}

lv_bool lvHashMap_iter(const lvHashMap *hashmap, size_t *index, void **item) {
    if (!hashmap) {
        return LV_FALSE;
    }

    do {
        if (*index >= hashmap->capacity) {
            return LV_FALSE;
        }

        *item = (void *)((char *)hashmap->data + (*index) * hashmap->item_size);
        (*index)++;

    } while(hashmap->data_state[*index] == LV_FALSE);

    return LV_TRUE;
}