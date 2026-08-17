#include <stdarg.h>
#include <stdio.h>
#include "lava/core/error.h"


static char error_buf[LV_ERROR_BUFFER_SIZE];

void _lv_throw(
    const char *file,
    unsigned int line,
    const char *message,
    ...
) {
    va_list args;
    va_start(args, message);

    char render_buf[LV_ERROR_BUFFER_SIZE];
    vsprintf(render_buf, message, args);

    va_end(args);

    sprintf(error_buf, "Error in %s, line %u: %s\n", file, line, render_buf);
}

char *lv_get_error() {
    return error_buf;
}