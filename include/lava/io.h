#ifndef LAVA_IO_H
#define LAVA_IO_H

#include "lava/internal.h"


typedef struct {
    size_t length;
    char *data;
} lvFileContent;

/**
 * @brief Read raw binary content from file.
 * 
 * lvFileContent.data is `NULL` if failed.
 * 
 * @param filepath Path to file.
 * @return lvFileContent
 */
lvFileContent lv_read_file_raw(const char *filepath);


#endif