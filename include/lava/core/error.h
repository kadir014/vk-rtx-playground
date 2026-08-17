#ifndef LAVA_ERROR_H
#define LAVA_ERROR_H


/* Size of the internal error message buffer in bytes. */
#define LV_ERROR_BUFFER_SIZE 512

void _lv_throw(
    const char *file,
    unsigned int line,
    const char *message,
    ...
);

/**
 * @brief Throw an error.
 * 
 * @param message String message describing the error.
 * @param ...
 */
#define LV_THROW(message, ...) _lv_throw(__FILE__, __LINE__, message, __VA_ARGS__)

/**
 * @brief Throw an error and return.
 * 
 * @param message String message describing the error.
 * @param ...
 */
#define LV_THROW_AND_RETURN(result, message, ...) {      \
    _lv_throw(__FILE__, __LINE__, message, __VA_ARGS__); \
    return result;                                       \
}

/**
 * @brief Throw an empty error (to clear the error buffer) and return.
 * 
 * This is useful when the error string message is not needed.
 */
#define LV_THROW_EMPTY_AND_RETURN(result) {  \
    _lv_throw(__FILE__, __LINE__, "%s", ""); \
    return result;                           \
}

#define LV_THROW_AND_RETURN_NULL(message, ...) {         \
    _lv_throw(__FILE__, __LINE__, message, __VA_ARGS__); \
    return NULL;                                         \
}

#define LV_THROW_EMPTY_AND_RETURN_NULL() {   \
    _lv_throw(__FILE__, __LINE__, "%s", ""); \
    return NULL;                             \
}

/**
 * @brief Get the last occured error message string.
 * 
 * @return char * 
 */
char *lv_get_error();


/**
 * @brief Common status codes returned commonly.
 * 
 * If the status code is indicating an error,
 * use @ref lv_get_error to get the detailed error message.
 */
typedef enum {
    lvResult_OK = 0,
    lvResult_FAILED_TO_ALLOCATE,
    lvResult_COULD_NOT_OPEN_FILE,
    lvResult_INVALID_ARGUMENTS,
} lvResult;


#endif // LAVA_ERROR_H