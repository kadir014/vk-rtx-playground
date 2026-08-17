#include "tinytest.h"
#include "lava/lava.h"

#include "units/array.h"
#include "units/refarray.h"
#include "units/hashmap.h"


int main(int argc, char *argv[]) {
    ttUnitTestSuite test = {0};
    test.colored_output = true;

    run_lvRefArray_tests(&test);
    run_lvArray_tests(&test);
    run_lvHashMap_tests(&test);

    tt_print_report(&test);

    lv_check_leaks();

    return 0;
}