#include "tinytest.h"
#include "lava/lava.h"


void test_lvRefArray_new(ttUnitTestSuite *test) {
    {
        lvRefArray refarray = lvRefArray_new();
        tt_expect(lvRefArray_valid(&refarray), test);
        tt_expect_size_t(refarray.size, 0, test);
        tt_expect_size_t(refarray.capacity, 1, test);
        tt_expect_float(refarray.growth_factor, 2.0f, test);
        lvRefArray_free(&refarray);
    }
    {
        lvRefArray refarray = lvRefArray_new_ex(5, 1.65f);
        tt_expect(lvRefArray_valid(&refarray), test);
        tt_expect_size_t(refarray.size, 0, test);
        tt_expect_size_t(refarray.capacity, 5, test);
        tt_expect_float(refarray.growth_factor, 1.65f, test);
        lvRefArray_free(&refarray);
    }
}

void test_lvRefArray_add(ttUnitTestSuite *test) {
    lvRefArray refarray = lvRefArray_new();

    int a = 42;
    lvRefArray_add(&refarray, &a);

    tt_expect_int(*(int *)refarray.data[0], 42, test);
    tt_expect_size_t(refarray.capacity, 1, test);
    tt_expect_size_t(refarray.size, 1, test);

    lvRefArray_free(&refarray);
}

void test_lvRefArray_pop(ttUnitTestSuite *test) {
    lvRefArray refarray = lvRefArray_new();

    int a = 42;
    lvRefArray_add(&refarray, &a);

    int b = 69;
    lvRefArray_add(&refarray, &b);

    int c = *(int *)lvRefArray_pop(&refarray, 0);

    tt_expect_int(c, 42, test);
    tt_expect_int(*(int *)refarray.data[0], 69, test);
    tt_expect_size_t(refarray.capacity, 2, test);
    tt_expect_size_t(refarray.size, 1, test);

    lvRefArray_free(&refarray);
}

void test_lvRefArray_remove(ttUnitTestSuite *test) {
    lvRefArray refarray = lvRefArray_new();

    int a = 42;
    lvRefArray_add(&refarray, &a);

    int b = 69;
    lvRefArray_add(&refarray, &b);

    size_t idx = lvRefArray_remove(&refarray, &a);

    tt_expect_size_t(idx, 0, test);
    tt_expect_int(*(int *)refarray.data[0], 69, test);
    tt_expect_size_t(refarray.capacity, 2, test);
    tt_expect_size_t(refarray.size, 1, test);

    size_t wrong_idx = lvRefArray_remove(&refarray, &a);
    tt_expect_size_t(wrong_idx, LV_INVALID_INDEX_ZU, test);

    lvRefArray_free(&refarray);
}

void test_lvRefArray_clear(ttUnitTestSuite *test) {
    lvRefArray refarray = lvRefArray_new();

    int a = 42;
    lvRefArray_add(&refarray, &a);

    int b = 69;
    lvRefArray_add(&refarray, &b);

    lvRefArray_clear(&refarray, NULL);

    tt_expect_size_t(refarray.capacity, 2, test);
    tt_expect_size_t(refarray.size, 0, test);

    lvRefArray_free(&refarray);
}

void test_lvRefArray_copy(ttUnitTestSuite *test) {
    lvRefArray refarray = lvRefArray_new();

    int a = 42;
    lvRefArray_add(&refarray, &a);

    int b = 69;
    lvRefArray_add(&refarray, &b);

    lvRefArray copy = lvRefArray_copy(&refarray);

    tt_expect_p(refarray.data[0], copy.data[0], test);
    tt_expect_p(refarray.data[1], copy.data[1], test);

    lvRefArray_free(&refarray);
    lvRefArray_free(&copy);
}


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

    tt_expect_int(*LV_ARRAY_AT(&array, 0, int), 42, test);
    tt_expect_size_t(array.capacity, 1, test);
    tt_expect_size_t(array.size, 1, test);

    lvArray_free(&array);
}


int main(int argc, char *argv[]) {
    ttUnitTestSuite test = {0};
    test.colored_output = true;

    TT_RUN_TEST(test_lvRefArray_new);
    TT_RUN_TEST(test_lvRefArray_add);
    TT_RUN_TEST(test_lvRefArray_pop);
    TT_RUN_TEST(test_lvRefArray_remove);
    TT_RUN_TEST(test_lvRefArray_clear);
    TT_RUN_TEST(test_lvRefArray_copy);

    TT_RUN_TEST(test_lvArray_new);
    TT_RUN_TEST(test_lvArray_add);

    tt_print_report(&test);

    return 0;
}