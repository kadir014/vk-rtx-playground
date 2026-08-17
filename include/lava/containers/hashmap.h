#ifndef LAVA_HASHMAP_H
#define LAVA_HASHMAP_H

#include "lava/internal.h"


/**
 * @brief User hasher function.
 * 
 * This should generate a hash for the given item,
 * it should be distributed as uniformly as possible to reduce collisions.
 * 
 * @param item Pointer to item.
 * @return 64-bit unsigned integer hash.
 */
typedef lv_u64 (*lvHashMap_hasher)(void *item);

/**
 * @brief User comparer function.
 * 
 * Determines whether two items are considered equal. This is used
 * during lookup and similar operations to identify an existing item.
 * 
 * @param a First item.
 * @param b Second item.
 * @return Result of user-defined comparison if two items are equal or not.
 */
typedef lv_bool (*lvHashMap_comparer)(void *a, void *b);

typedef struct {
    size_t size; /**< Current number of elements in the hashmap. */
    size_t capacity; /**< Current allocated space for maximum number of elements. */
    size_t item_size; /**< Size of one item in bytes. */
    float growth_factor; /**< Capacity multipler used during reallocations. Must be higher than 1. */
    float growth_rule; /**< Capacity percentage to decide when to resize. Must be in range (0, 1]. */
    void *data; /**< Item array. */
    lv_bool *data_state; /**< Occupancy array. */
    lvHashMap_hasher hasher;
    lvHashMap_comparer comparer;
} lvHashMap;

/**
 * @brief Create a new hashmap.
 * 
 * Use @ref lvHashMap_valid to see if creation was successful. If failed,
 * use @ref lv_get_error for more information.
 * 
 * Usage:
 * ```
 * typedef struct {
 *     char name[16];
 *     char info[256];
 *     int age;
 * } MyEntry;
 * 
 * lv_u64 my_hasher(void *item) {
 *     MyEntry *entry = item;
 * 
 *     return some_64bit_hash_function(entry.name);
 * }
 * 
 * lv_bool my_comparer(void *a, void *b) {
 *     MyEntry *entry_a = a;
 *     MyEntry *entry_b = b;
 *     
 *     return strcmp(entry_a->name, entry_b->name) == 0;
 * }
 * 
 * lvHashMap mymap = lvHashMap_new(sizeof(MyEntry), my_hasher);
 * ```
 * 
 * @param item_size Size of one item in bytes.
 * @param hasher User hasher function, see @ref lvHashMap_hasher for details.
 * @param comparer User comparer function, see @ref lvHashMap_comparer for details.
 * @return lvHashMap
 */
lvHashMap lvHashMap_new(
    size_t item_size,
    lvHashMap_hasher hasher,
    lvHashMap_comparer comparer
);

/**
 * @brief Create a new hashmap.
 * 
 * Use @ref lvHashMap_valid to see if creation was successful. If failed,
 * use @ref lv_get_error for more information.
 * 
 * Refer to @ref lvHashMap_new for example usage.
 * 
 * @param item_size Size of one item in bytes.
 * @param hasher User hasher function, see @ref lvHashMap_hasher for details.
 * @param comparer User comparer function, see @ref lvHashMap_comparer for details.
 * @param default_capacity Initial number of items to allocate for.
 * @param growth_factor Capacity multipler used during reallocations. Must be higher than 1.
 * @param growth_rule Capacity percentage to decide when to resize. Must be in range (0, 1].
 * @return lvHashMap 
 */
lvHashMap lvHashMap_new_ex(
    size_t item_size,
    lvHashMap_hasher hasher,
    lvHashMap_comparer comparer,
    size_t default_capacity,
    float growth_factor,
    float growth_rule
);

/**
 * @brief Destroy the hashmap.
 * 
 * It's safe to pass `NULL` to this function.
 * 
 * @param hashmap 
 */
void lvHashMap_free(lvHashMap *hashmap);

/**
 * @brief Check if hashmap is valid.
 * 
 * @param hashmap Hashmap.
 * @return Whether the state is valid or not.
 */
lv_bool lvHashMap_valid(const lvHashMap *hashmap);

/**
 * @brief Clear the contents of the hashmap.
 * 
 * @note The space is not reallocated, only the elements are cleared.
 * 
 * @param hashmap Hashmap.
 */
void lvHashMap_clear(lvHashMap *hashmap);

/**
 * @brief Fetch item from the hashmap.
 * 
 * Only the key member must be initialized in the given item.
 * 
 * @param hashmap Hashmap.
 * @param item Pointer to item.
 * @return Pointer to item inside hashmap.
 *         `NULL` if failed, use @ref lv_get_error for more information.
 */
void *lvHashMap_get(const lvHashMap *hashmap, void *item);

/**
 * @brief Set an existing item or add new one to hashmap.
 * 
 * Only the key member must be initialized in the given item.
 * 
 * @param hashmap Hashmap.
 * @param item Pointer to item.
 * @return Pointer to item inside hashmap.
 *         `NULL` if failed, use @ref lv_get_error for more information.
 */
void *lvHashMap_set(lvHashMap *hashmap, void *item);

/**
 * @brief Remove an existing item from hashmap.
 * 
 * Only the key member must be initialized in the given item.
 * 
 * @param hashmap Hashmap.
 * @param item Pointer to item.
 * @return TODO
 */
lvResult lvHashMap_remove(lvHashMap *hashmap, void *item);

/**
 * @brief Check whether an item exists inside hashmap.
 * 
 * Only the key member must be initialized in the given item.
 * 
 * @param hashmap Hashmap.
 * @param item Pointer to item.
 * @return Whether the hashmap contains the item or not.
 */
lv_bool lvHashMap_contains(const lvHashMap *hashmap, void *item);

/**
 * @brief Iterate every item in the hashmap in no definite order.
 * 
 * @warning Do not mutate the hashmap while iterating.
 * 
 * Usage:
 * ```
 * size_t idx = 0;
 * void *item = NULL;
 * 
 * while (lvHashMap_iter(&my_hashmap, &idx, &item)) {
 *     // Do stuff with 'item'...
 * }
 * ```
 * 
 * @param hashmap Hashmap.
 * @param index Pointer to set the current index.
 * @param item Pointer to set the current item.
 * @return Is the iteration still running?
 */
lv_bool lvHashMap_iter(const lvHashMap *hashmap, size_t *index, void **item);


#endif // LAVA_HASHMAP_H