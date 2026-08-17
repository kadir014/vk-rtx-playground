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
 * It's caller's responsibility to free `lvFileContent->data`.
 * 
 * @param filepath Path to file.
 * @param content Pointer to content struct.
 * @return Returns `lvResult_OK` if successful.
 */
lvResult lv_read_file_raw(const char *filepath, lvFileContent *content);


#endif // LAVA_IO_H