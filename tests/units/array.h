#include "tinytest.h"
#include "lava/lava.h"


void test_lvArray_new(ttUnitTestSuite *test) {
    {
        lvArray array = lvArray_new(sizeof(int));
        tt_expect(lvArray_valid(&array), test);
        tt_expect_size_t(array.size, 0, test);
        tt_expect_size_t(array.capacity, 1, test);
        tt_expect_float(array.growth_factor, 2.0f, test);
        lvArray_free(&array);
    }
    {
        lvArray array = lvArray_new_ex(sizeof(int), 5, 1.65f);
        tt_expect(lvArray_valid(&array), test);
        tt_expect_size_t(array.size, 0, test);
        tt_expect_size_t(array.capacity, 5, test);
        tt_expect_float(array.growth_factor, 1.65f, test);
        lvArray_free(&array);
    }
}

void test_lvArray_add(ttUnitTestSuite *test) {
    lvArray array = lvArray_new(sizeof(int));

    int a = 42;
    lvArray_add(&array, &a);

    tt_expect_int(LV_ARRAY_AT(&array, 0, int), 42, test);
    tt_expect_size_t(array.capacity, 1, test);
    tt_expect_size_t(array.size, 1, test);

    lvArray_free(&array);
}

void test_lvArray_resize(ttUnitTestSuite *test) {
    lvArray array = lvArray_new(sizeof(int));

    array.size = 31;
    lvArray_resize(&array);

    tt_expect(lvArray_valid(&array), test);
    tt_expect_size_t(array.capacity, 31, test);
    tt_expect_size_t(array.size, 31, test);

    lvArray_free(&array);
}


void run_lvArray_tests(ttUnitTestSuite *test) {
    TT_RUN_TEST_P(test_lvArray_new);
    TT_RUN_TEST_P(test_lvArray_add);
    TT_RUN_TEST_P(test_lvArray_resize);
}