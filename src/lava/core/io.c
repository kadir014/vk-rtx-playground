#include <stdio.h>
#include "lava/core/io.h"


lvFileContent lv_read_file_raw(const char *filepath) {
    lvFileContent cont = {.length = 0, .data = NULL};

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        return cont;
    }

    // Seek to the end & rewind back to determine the file size
    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    rewind(file);

    char *buffer = LV_MALLOC(length + 1);
    if (!buffer) {
        fclose(file);
        return cont;
    }

    fread(buffer, 1, length, file);
    // Make sure to null-terminate the content
    buffer[length] = '\0';

    fclose(file);
    
    cont.length = length;
    cont.data = buffer;
    return cont;
}