#ifndef TINYTEST_H
#define TINYTEST_H

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>
#include <string.h>


/**
 * @file tinytest.h
 * 
 * @brief A tiny header-only unit test library I copy around for my personal projects :)
 * 
 * Features:
 * - Portable C99, No dependencies.
 * - Optional colored output mode.
 * - Reports every failed test and the parameters.
 * - Define your own TT_DBL_ERROR to determine float comparison error.
 * - Internal functions start with an underscore, rest is public API.
 * - Expect condition (tt_expect).
 * - Expect floats and doubles (tt_expect_[float, double]).
 * - Expect strings (tt_expect_string)
 * - Expect pointer addresses (tt_expect_p).
 * - Expect all integral types (tt_expect_[int type]).
 * 
 * Example usage:
 * 
 *     #include "tinytest.h"
 *     #include "some_library.h"
 *     
 *     void buffer_creation(ttUnitTestSuite *test) {
 *         Buffer *buffer = CreateBuffer(24);
 *     
 *         tt_expect(buffer->size > 16, test);
 *         tt_expect_float(buffer->factor, 2.0f, test);
 *         tt_expect_uint64_t(buffer->capacity, 1024, test);
 *     }
 *     
 *     int main() {
 *         // You can choose whether to use colored output or not
 *         ttUnitTestSuite test = {0};
 *         test.colored_output = true;
 *     
 *         // Execute unit tests
 *         TT_RUN_TEST(buffer_creation);
 *     
 *         // Print the final test report
 *         tt_print_report(&test);
 *     
 *         return 0;
 *     }
 */


#define TT_ANSI_END           "\x1B[0m"
#define TT_ANSI_UNDERLINE     "\x1B[04m"

#define TT_ANSI_FG_BLACK      "\x1B[0;30m"
#define TT_ANSI_FG_DARKGRAY   "\x1B[0;90m"
#define TT_ANSI_FG_LIGHTGRAY  "\x1B[0;37m"
#define TT_ANSI_FG_WHITE      "\x1B[0;97m"
#define TT_ANSI_FG_RED        "\x1B[0;31m"
#define TT_ANSI_FG_ORANGE     "\x1B[0;33m"
#define TT_ANSI_FG_YELLOW     "\x1B[0;93m"
#define TT_ANSI_FG_GREEN      "\x1B[0;32m"
#define TT_ANSI_FG_BLUE       "\x1B[0;34m"
#define TT_ANSI_FG_CYAN       "\x1B[0;36m"
#define TT_ANSI_FG_PURPLE     "\x1B[0;35m"
#define TT_ANSI_FG_MAGENTA    "\x1B[0;95m"
#define TT_ANSI_FG_LIGHTRED   "\x1B[0;91m"
#define TT_ANSI_FG_LIGHTGREEN "\x1B[0;92m"
#define TT_ANSI_FG_LIGHTBLUE  "\x1B[0;94m"
#define TT_ANSI_FG_LIGHTCYAN  "\x1B[0;96m"

#define TT_ANSI_BG_BLACK      "\x1B[0;40m"
#define TT_ANSI_BG_DARKGRAY   "\x1B[0;100m"
#define TT_ANSI_BG_LIGHTGRAY  "\x1B[0;47m"
#define TT_ANSI_BG_WHITE      "\x1B[0;107m"
#define TT_ANSI_BG_RED        "\x1B[0;41m"
#define TT_ANSI_BG_ORANGE     "\x1B[0;43m"
#define TT_ANSI_BG_YELLOW     "\x1B[0;103m"
#define TT_ANSI_BG_GREEN      "\x1B[0;42m"
#define TT_ANSI_BG_BLUE       "\x1B[0;44m"
#define TT_ANSI_BG_CYAN       "\x1B[0;46m"
#define TT_ANSI_BG_PURPLE     "\x1B[0;45m"
#define TT_ANSI_BG_MAGENTA    "\x1B[0;105m"
#define TT_ANSI_BG_LIGHTRED   "\x1B[0;101m"
#define TT_ANSI_BG_LIGHTGREEN "\x1B[0;102m"
#define TT_ANSI_BG_LIGHTBLUE  "\x1B[0;104m"
#define TT_ANSI_BG_LIGHTCYAN  "\x1B[0;106m"


/* Accepted float precision error for tests. */
#ifndef TT_DBL_ERROR
    #define TT_DBL_ERROR ((double)(0.000001))
#endif
#define TT_FLT_ERROR ((float)(TT_DBL_ERROR))


/**
 * @brief Unit test suite context.
 */
typedef struct {
    char *current; /**< Name of the current function that is being tested. */
    uint32_t total; /**< Amount of tests done so far. */
    uint32_t fails; /**< Failed tests so far. */
    bool colored_output; /**< Whether to use ANSI codes to color the output. */
    int32_t internal_tests;
} ttUnitTestSuite;

/* Update the total of tests done. */
#define _TT_UPDATE_TOTAL {test->total++; test->internal_tests++;}

/* Update the amount of failed tests. */
#define _TT_UPDATE_FAILS (test->fails++)


/**
 * @brief Execute a unit test function.
 * 
 * @note You must have a unit test context initialized with the name `test` in the current scope.
 * 
 * @param func Function to execute.
 */
#define TT_RUN_TEST(func) {test.current = #func; test.internal_tests = -1; ##func(&test);}

/**
 * @brief Print a final report of total unit tests ran.
 * 
 * @param test Pointer to unit test context.
 */
void tt_print_report(const ttUnitTestSuite *test) {
    printf("\n──────────────────────────────────────────────────\n\n");

    uint32_t passed = test->total - test->fails;
    double ftotal = (double)test->total;
    double ffails = (double)test->fails;
    double fpassed = (double)passed;

    double fails_perc = ffails / ftotal * 100.0;
    double passed_perc = fpassed / ftotal * 100.0;

    if (!test->colored_output) {
        printf("Total tests: %u\n", test->total);
        printf("Failed:      %u (%.1f %%)\n", test->fails, fails_perc);
        printf("Passed:      %u (%.1f %%)\n", passed, passed_perc);
    }
    else {
        printf(
            "Total tests: " TT_ANSI_FG_YELLOW "%u" TT_ANSI_END "\n",
            test->total
        );
        printf(
            TT_ANSI_FG_LIGHTRED "Failed:      " TT_ANSI_FG_YELLOW "%u" TT_ANSI_END " (%.1f %%)\n",
            test->fails, fails_perc
        );
        printf(
            TT_ANSI_FG_LIGHTGREEN "Passed:      " TT_ANSI_FG_YELLOW "%u" TT_ANSI_END " (%.1f %%)\n",
            passed, passed_perc
        );
    }
}


static char _tt_identifier_buf[64];

static inline char *_tt_test_identifier(ttUnitTestSuite *test) {
    if (!test->colored_output) {
        sprintf(_tt_identifier_buf, "%s#%d", test->current, test->internal_tests);
    }
    else {
        sprintf(
            _tt_identifier_buf, "%s" TT_ANSI_FG_DARKGRAY "#" TT_ANSI_FG_LIGHTBLUE "%d" TT_ANSI_END,
            test->current, test->internal_tests
        );
    }
    return _tt_identifier_buf;
}

void _tt_print_passed(ttUnitTestSuite *test) {
    if (!test->colored_output) {
        printf("[PASSED] %s\n", _tt_test_identifier(test));
    }
    else {
        printf(
            TT_ANSI_FG_DARKGRAY "[" TT_ANSI_FG_LIGHTGREEN "PASSED" TT_ANSI_FG_DARKGRAY "]" TT_ANSI_END " %s\n",
            _tt_test_identifier(test)
        );
    }
}

void _tt_print_failed(ttUnitTestSuite *test, const char *msg) {
    if (!test->colored_output) {
        printf(
            "[FAILED] %s: %s\n",
            _tt_test_identifier(test), msg
        );
    }
    else {
        printf(
            TT_ANSI_FG_DARKGRAY "[" TT_ANSI_FG_LIGHTRED "FAILED" TT_ANSI_FG_DARKGRAY "]" TT_ANSI_END " %s" TT_ANSI_FG_DARKGRAY ":" TT_ANSI_END " %s\n",
            _tt_test_identifier(test), msg
        );
    }
}

#define _tt_print_failed_type(test, type, fmt) {                       \
    if (!test->colored_output) {                                       \
        printf(                                                        \
            "[FAILED] %s: Expected (%s)" fmt " but got (%s)" fmt "\n", \
            _tt_test_identifier(test), #type, expect, #type, value     \
        );                                                             \
    } else {                                                           \
        printf(                                                        \
            TT_ANSI_FG_DARKGRAY "[" TT_ANSI_FG_LIGHTRED "FAILED" TT_ANSI_FG_DARKGRAY "]" TT_ANSI_END " %s" TT_ANSI_FG_DARKGRAY ":" TT_ANSI_END " Expected " TT_ANSI_FG_DARKGRAY "(" TT_ANSI_FG_YELLOW "%s" TT_ANSI_FG_DARKGRAY ")" TT_ANSI_FG_LIGHTGREEN fmt TT_ANSI_END " but got " TT_ANSI_FG_DARKGRAY "(" TT_ANSI_FG_YELLOW "%s" TT_ANSI_FG_DARKGRAY ")" TT_ANSI_FG_LIGHTRED fmt TT_ANSI_END "\n", \
            _tt_test_identifier(test), #type, expect, #type, value     \
        );                                                             \
    }                                                                  \
}


/**
 * @brief Expect a condition.
 * 
 * @param condition Condition to expect to be true.
 * @param test Pointer to unit test context.
 */
#define tt_expect(condition, test) _tt_expect(test, #condition, condition);

void _tt_expect(ttUnitTestSuite *test, const char *msg, bool condition) {
    _TT_UPDATE_TOTAL;

    if (condition) {
        _tt_print_passed(test);
    }
    else {
        _tt_print_failed(test, msg);
        _TT_UPDATE_FAILS;
    }
}

/**
 * @brief Compare two floats.
 * 
 * @param value Current value.
 * @param expect Expected value.
 * @param test Pointer to unit test context.
 */
void tt_expect_float(float value, float expect, ttUnitTestSuite *test) {
    _TT_UPDATE_TOTAL;

    if (fabsf(expect - value) <= TT_FLT_ERROR) {
        _tt_print_passed(test);
    }
    else {
        _tt_print_failed_type(test, float, "%f");
        _TT_UPDATE_FAILS;
    }
}

/**
 * @brief Compare two doubles.
 * 
 * @param value Current value.
 * @param expect Expected value.
 * @param test Pointer to unit test context.
 */
void tt_expect_double(double value, double expect, ttUnitTestSuite *test) {
    _TT_UPDATE_TOTAL;

    if (fabsf(expect - value) <= TT_DBL_ERROR) {
        _tt_print_passed(test);
    }
    else {
        _tt_print_failed_type(test, double, "%f");
        _TT_UPDATE_FAILS;
    }
}

/**
 * @brief Compare two strings (char arrays).
 * 
 * @param value Current value.
 * @param expect Expected value.
 * @param test Pointer to unit test context.
 */
void tt_expect_string(const char *value, const char *expect, ttUnitTestSuite *test) {
    _TT_UPDATE_TOTAL;

    if (strcmp(value, expect) == 0) {
        _tt_print_passed(test);
    }
    else {
        _tt_print_failed_type(test, char *, "%s");
        _TT_UPDATE_FAILS;
    }
}

/**
 * @brief Compare pointers.
 * 
 * @note This only compares the address values, not the data at the pointed memory. 
 * 
 * @param value Current value.
 * @param expect Expected value.
 * @param test Pointer to unit test context.
 */
void tt_expect_p(const void *value, const void *expect, ttUnitTestSuite *test) {
    _TT_UPDATE_TOTAL;

    if (value == expect) {
        _tt_print_passed(test);
    }
    else {
        _tt_print_failed_type(test, void *, "%p");
        _TT_UPDATE_FAILS;
    }
}


#define _TT_DEFINE_INTEGRAL_EXPECT(name, type, fmt)                   \
void tt_expect_##name(type value, type expect, ttUnitTestSuite *test) \
{                                                                     \
    _TT_UPDATE_TOTAL;                                                 \
                                                                      \
    if (value == expect) {                                            \
        _tt_print_passed(test);                                       \
    } else {                                                          \
        _tt_print_failed_type(test, type, fmt);                       \
        _TT_UPDATE_FAILS;                                             \
    }                                                                 \
}

/* Standard integer types */

_TT_DEFINE_INTEGRAL_EXPECT(char,               char,               "%c")
_TT_DEFINE_INTEGRAL_EXPECT(signed_char,        signed char,        "%hhd")
_TT_DEFINE_INTEGRAL_EXPECT(unsigned_char,      unsigned char,      "%hhu")

_TT_DEFINE_INTEGRAL_EXPECT(short,              short,              "%hd")
_TT_DEFINE_INTEGRAL_EXPECT(unsigned_short,     unsigned short,     "%hu")

_TT_DEFINE_INTEGRAL_EXPECT(int,                int,                "%d")
_TT_DEFINE_INTEGRAL_EXPECT(unsigned_int,       unsigned int,       "%u")

_TT_DEFINE_INTEGRAL_EXPECT(long,               long,               "%ld")
_TT_DEFINE_INTEGRAL_EXPECT(unsigned_long,      unsigned long,      "%lu")

_TT_DEFINE_INTEGRAL_EXPECT(long_long,          long long,          "%lld")
_TT_DEFINE_INTEGRAL_EXPECT(unsigned_long_long, unsigned long long, "%llu")

/* stdint.h exact-width integer types */

_TT_DEFINE_INTEGRAL_EXPECT(int8_t,             int8_t,             "%" PRId8)
_TT_DEFINE_INTEGRAL_EXPECT(uint8_t,            uint8_t,            "%" PRIu8)

_TT_DEFINE_INTEGRAL_EXPECT(int16_t,            int16_t,            "%" PRId16)
_TT_DEFINE_INTEGRAL_EXPECT(uint16_t,           uint16_t,           "%" PRIu16)

_TT_DEFINE_INTEGRAL_EXPECT(int32_t,            int32_t,            "%" PRId32)
_TT_DEFINE_INTEGRAL_EXPECT(uint32_t,           uint32_t,           "%" PRIu32)

_TT_DEFINE_INTEGRAL_EXPECT(int64_t,            int64_t,            "%" PRId64)
_TT_DEFINE_INTEGRAL_EXPECT(uint64_t,           uint64_t,           "%" PRIu64)

/* stdint.h least-width integer types */

_TT_DEFINE_INTEGRAL_EXPECT(int_least8_t,       int_least8_t,       "%" PRIdLEAST8)
_TT_DEFINE_INTEGRAL_EXPECT(uint_least8_t,      uint_least8_t,      "%" PRIuLEAST8)

_TT_DEFINE_INTEGRAL_EXPECT(int_least16_t,      int_least16_t,      "%" PRIdLEAST16)
_TT_DEFINE_INTEGRAL_EXPECT(uint_least16_t,     uint_least16_t,     "%" PRIuLEAST16)

_TT_DEFINE_INTEGRAL_EXPECT(int_least32_t,      int_least32_t,      "%" PRIdLEAST32)
_TT_DEFINE_INTEGRAL_EXPECT(uint_least32_t,     uint_least32_t,     "%" PRIuLEAST32)

_TT_DEFINE_INTEGRAL_EXPECT(int_least64_t,      int_least64_t,      "%" PRIdLEAST64)
_TT_DEFINE_INTEGRAL_EXPECT(uint_least64_t,     uint_least64_t,     "%" PRIuLEAST64)

/* stdint.h fastest integer types */

_TT_DEFINE_INTEGRAL_EXPECT(int_fast8_t,        int_fast8_t,        "%" PRIdFAST8)
_TT_DEFINE_INTEGRAL_EXPECT(uint_fast8_t,       uint_fast8_t,       "%" PRIuFAST8)

_TT_DEFINE_INTEGRAL_EXPECT(int_fast16_t,       int_fast16_t,       "%" PRIdFAST16)
_TT_DEFINE_INTEGRAL_EXPECT(uint_fast16_t,      uint_fast16_t,      "%" PRIuFAST16)

_TT_DEFINE_INTEGRAL_EXPECT(int_fast32_t,       int_fast32_t,       "%" PRIdFAST32)
_TT_DEFINE_INTEGRAL_EXPECT(uint_fast32_t,      uint_fast32_t,      "%" PRIuFAST32)

_TT_DEFINE_INTEGRAL_EXPECT(int_fast64_t,       int_fast64_t,       "%" PRIdFAST64)
_TT_DEFINE_INTEGRAL_EXPECT(uint_fast64_t,      uint_fast64_t,      "%" PRIuFAST64)

/* Word-sized integers */

_TT_DEFINE_INTEGRAL_EXPECT(intptr_t,           intptr_t,           "%" PRIdPTR)
_TT_DEFINE_INTEGRAL_EXPECT(uintptr_t,          uintptr_t,          "%" PRIuPTR)
_TT_DEFINE_INTEGRAL_EXPECT(size_t,             size_t,             "%zu")
_TT_DEFINE_INTEGRAL_EXPECT(ptrdiff_t,          ptrdiff_t,          "%td")


#endif // TINYTEST_H