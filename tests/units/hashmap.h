#include "tinytest.h"
#include "lava/lava.h"


static inline lv_u32 wang_hash_u32(lv_u32 a) {
    // https://burtleburtle.net/bob/hash/integer.html
    a = (a ^ 61u) ^ (a >> 16u);
    a = a + (a << 3u);
    a = a ^ (a >> 4u);
    a = a * 0x27d4eb2du;
    a = a ^ (a >> 15u);
    return a;
}

typedef struct {
    lv_u32 id;
    char *name;
} TestEntry;

lv_u64 test_hasher(void *item) {
    TestEntry *entry = item;

    return (lv_u64)wang_hash_u32(entry->id);
}

lv_bool test_comparer(void *a, void *b) {
    TestEntry *entry_a = a;
    TestEntry *entry_b = b;

    return entry_a->id == entry_b->id;
}

void test_lvHashMap_new(ttUnitTestSuite *test) {
    {
        lvHashMap hashmap = lvHashMap_new(sizeof(TestEntry), test_hasher, test_comparer);
        tt_expect(lvHashMap_valid(&hashmap), test);
        tt_expect_size_t(hashmap.size, 0, test);
        lvHashMap_free(&hashmap);
    }

    {
        lvHashMap hashmap = lvHashMap_new_ex(sizeof(TestEntry), test_hasher, test_comparer, 42, 3.0f, 0.75f);
        tt_expect(lvHashMap_valid(&hashmap), test);
        tt_expect_size_t(hashmap.size, 0, test);
        tt_expect_size_t(hashmap.capacity, 42, test);
        tt_expect_float(hashmap.growth_factor, 3.0f, test);
        tt_expect_float(hashmap.growth_rule, 0.75f, test);
        lvHashMap_free(&hashmap);
    }
}

void test_lvHashMap_set(ttUnitTestSuite *test) {
    lvHashMap hashmap = lvHashMap_new(sizeof(TestEntry), test_hasher, test_comparer);

    void *item = lvHashMap_set(&hashmap, &(TestEntry){.id = 42, .name = "Hey!"});
    tt_expect(item != NULL, test);

    TestEntry *entry = item;
    tt_expect_uint32_t(entry->id, 42, test);
    tt_expect_string(entry->name, "Hey!", test);

    lvHashMap_free(&hashmap);
}

void test_lvHashMap_get(ttUnitTestSuite *test) {
    lvHashMap hashmap = lvHashMap_new(sizeof(TestEntry), test_hasher, test_comparer);

    lvHashMap_set(&hashmap, &(TestEntry){.id = 42, .name = "Hey!"});
    
    void *item0 = lvHashMap_get(&hashmap, &(TestEntry){.id = 42});
    tt_expect(item0 != NULL, test);

    TestEntry *entry0 = item0;
    tt_expect_uint32_t(entry0->id, 42, test);
    tt_expect_string(entry0->name, "Hey!", test);

    void *item1 = lvHashMap_get(&hashmap, &(TestEntry){0});
    tt_expect(item1 == NULL, test);

    void *item2 = lvHashMap_get(&hashmap, &(TestEntry){.name = "Hey!"});
    tt_expect(item2 == NULL, test);

    lvHashMap_free(&hashmap);
}

void test_lvHashMap_stress_get(ttUnitTestSuite *test) {
    lvHashMap hashmap = lvHashMap_new(sizeof(TestEntry), test_hasher, test_comparer);

    for (lv_u32 i = 0; i < 1000000; i++) {
        lvHashMap_set(&hashmap, &(TestEntry){.id = i, .name = "Hey!"});
    }

    void *item0 = lvHashMap_get(&hashmap, &(TestEntry){.id = 0});
    tt_expect(item0 != NULL, test);

    void *item1 = lvHashMap_get(&hashmap, &(TestEntry){.id = 1000000 - 1});
    tt_expect(item1 != NULL, test);

    void *item2 = lvHashMap_get(&hashmap, &(TestEntry){.id = 1000000});
    tt_expect(item2 == NULL, test);

    lvHashMap_free(&hashmap);
}


void run_lvHashMap_tests(ttUnitTestSuite *test) {
    TT_RUN_TEST_P(test_lvHashMap_new);
    TT_RUN_TEST_P(test_lvHashMap_set);
    TT_RUN_TEST_P(test_lvHashMap_get);
    TT_RUN_TEST_P(test_lvHashMap_stress_get);
}