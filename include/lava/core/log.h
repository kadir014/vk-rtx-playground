#ifndef LAVA_LOG_H
#define LAVA_LOG_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


/**
 * @brief Log fatal message and exit with failure status code.
 * 
 * @param fmt Formatter string for the message
 * @param ... 
 */
static inline void lv_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    printf("[FATAL] ");
    vprintf(fmt, args);
    printf("\n");

    va_end(args);

    exit(EXIT_FAILURE);
}


#endif // LAVA_LOG_H