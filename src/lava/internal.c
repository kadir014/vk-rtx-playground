#include "lava/internal.h"


#define PRINT_SEPARATOR "──────────────────────────────────────────────────"
#define INITIAL_CAPACITY 8192
#define GROWTH_FACTOR 3.0f


/**
 * @brief Allocation tracking data.
 */
typedef struct {
    void *ptr; /**< Pointer to block. */
    size_t size; /**< Size requested for allocation. */
    size_t reallocs; /**< How many times did this block get reallocated. */
    const char *file; /**< File path to the source. */
    uint32_t line; /**< Line number in source. */
} Allocation;

static Allocation *g_allocs;
static size_t g_size = 0;
static size_t g_capacity = 0;

static void ensure_initialized() {
    if (g_capacity > 0) return;

    g_capacity = INITIAL_CAPACITY;
    // calloc instead of malloc for more readible output in debugger monitors
    g_allocs = calloc(g_capacity, sizeof(Allocation));
}

static bool add(Allocation *alloc) {
    if (g_size == g_capacity) {
        size_t new_capacity = (size_t)((float)g_capacity * GROWTH_FACTOR);

        Allocation *new_allocs = realloc(
            g_allocs,
            sizeof(Allocation) * new_capacity
        );

        if (!new_allocs) {
            return false;
        }

        g_allocs = new_allocs;
        g_capacity = new_capacity;
    }

    g_allocs[g_size++] = *alloc;

    return true;
}
static void pop(size_t index) {
    if (g_size == 0 || index >= g_size) {
        return;
    }

    // Instead of shifting everything, we just switch places with last element
    // But order is not a priority, so this is faster.
    g_size--;

    g_allocs[index] = g_allocs[g_size];
    g_allocs[g_size] = (Allocation){0};
}

void *_lv_malloc(size_t size, const char *file, uint32_t line) {
    ensure_initialized();

    void *ptr = malloc(size);

    // If malloc failed, don't bother adding to array
    if (!ptr) {
        return NULL;
    }

    Allocation alloc = {
        .ptr = ptr,
        .size = size,
        .reallocs = 0,
        .file = file,
        .line = line
    };

    if (!add(&alloc)) {
        if (ptr) {
            free(ptr);
        }

        return NULL;
    }

    return ptr;
}

void *_lv_realloc(void *ptr, size_t new_size, const char *file, uint32_t line) {
    ensure_initialized();

    // realloc(NULL, ...) is valid in C
    if (ptr == NULL) {
        return _lv_malloc(new_size, file, line);
    }

    // realloc(..., 0) is problematic
    if (new_size == 0) {
        _lv_free(ptr, file, line);
        return NULL;
    }

    bool found = false;
    size_t found_idx = 0;
    for (size_t i = 0; i < g_size; i++) {
        if (g_allocs[i].ptr == ptr) {
            found = true;
            found_idx = i;
            break;
        }
    }

    if (!found) {
        printf(
            "[LV_DEBUG] realloc requested but pointer was not allocated before or already freed!\n"
            "- Pointer:  0x%p\n"
            "- New size: %zu\n"
            "- File:     '%s'\n"
            "- Line:     %u\n"
            "\n",
            ptr,
            new_size,
            file,
            line
        );
        return NULL;
    }

    void *new_ptr = realloc(ptr, new_size);

    // If realloc failed, original block should stay allocated, don't remove it
    if (!new_ptr) {
        return NULL;
    }

    g_allocs[found_idx].ptr = new_ptr;
    g_allocs[found_idx].size = new_size;
    g_allocs[found_idx].reallocs++;

    return new_ptr;
}

void _lv_free(void *ptr, const char *file, uint32_t line) {
    ensure_initialized();

    bool found = false;
    size_t found_idx = 0;
    for (size_t i = 0; i < g_size; i++) {
        if (g_allocs[i].ptr == ptr) {
            found = true;
            found_idx = i;
            break;
        }
    }

    if (!found) {
        printf(
            "[LV_DEBUG] free requested but pointer was not allocated before or already freed!\n"
            "- Pointer: 0x%p\n"
            "- File:    '%s'\n"
            "- Line:    %u\n"
            "\n",
            ptr,
            file,
            line
        );
        return;
    }

    free(ptr);

    pop(found_idx);
}

void lv_check_leaks() {
    #ifdef LV_DEBUG

        if (g_size == 0) {
            printf("\n[LV_DEBUG] There are no leaked allocations.\n");

            free(g_allocs);
            g_allocs = NULL;
            g_size = 0;
            g_capacity = 0;

            return;
        }

        printf(
            "\n"
            "[LV_DEBUG] There are %zu leaked allocations.\n"
            PRINT_SEPARATOR "\n"
            "\n",
            g_size
        );

        for (size_t i = 0; i < g_size; i++) {
            Allocation alloc = g_allocs[i];

            printf(
                "Allocation #%zu:\n"
                "- Pointer:  0x%p\n"
                "- Size:     %zu bytes\n"
                "- Reallocs: %zu times\n"
                "- File:     '%s'\n"
                "- Line:     %u\n"
                "\n",
                i,
                alloc.ptr,
                alloc.size,
                alloc.reallocs,
                alloc.file,
                alloc.line
            );
        }

        printf(
            PRINT_SEPARATOR "\n"
            "[LV_DEBUG] Finished allocation report.\n",
            g_size
        );

        free(g_allocs);
        g_allocs = NULL;
        g_size = 0;
        g_capacity = 0;

    #else

        printf("\n[LV_DEBUG] Memory tracking is disabled in non-debug build.\n");

    #endif
}