#include <stdio.h>
#include "lava/core/io.h"


lvResult lv_read_file_raw(const char *filepath, lvFileContent *content) {
    content->length = 0;
    content->data = NULL;

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        return lvResult_COULD_NOT_OPEN_FILE;
    }

    // Seek to the end & rewind back to determine the file size
    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    rewind(file);

    char *buffer = LV_MALLOC(length + 1);
    if (!buffer) {
        fclose(file);
        return lvResult_FAILED_TO_ALLOCATE;
    }

    fread(buffer, 1, length, file);
    // Make sure to null-terminate the content
    buffer[length] = '\0';

    fclose(file);
    
    content->length = length;
    content->data = buffer;
    
    return lvResult_OK;
}