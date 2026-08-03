#include "lava/loaders/obj.h"
#include "lava/core/io.h"


/*
    The reason these 'fast' parsing functions are faster than standard library's
    alternatives is that they only work on a very small subset of numerals.

    For example they only suppert decimal representation, no scientific notation,
    no constants like NaN or inf, no overflow checking etc...

    Since OBJ files also store only those boring subset of numbers, we can
    utilize these faster alternatives.
*/

/**
 * @brief Fast ASCII string to 32-bit float.
 * 
 * Advances the given pointer to string.
 * 
 * @param p Pointer to string (char array).
 * @return float 
 */
static inline float fast_str2f(const char **p) {
    const char *s = *p;

    float sign = 1.0f;
    if (*s == '-') {
        sign = -1.0f;
        s++;
    }
    else if (*s == '+') {
        s++;
    }

    // INTEGER PART
    float value = 0.0f;
    while (*s >= '0' && *s <= '9') {
        value = value * 10.0f + (*s - '0');
        s++;
    }

    // FRACTIONAL PART
    if (*s == '.') {
        s++;
        float scale = 0.1f;
        while (*s >= '0' && *s <= '9') {
            value += (*s - '0') * scale;
            // Each successive digit contributes one-tenth as much.
            // same as moving to next decimal place.
            scale *= 0.1f;
            s++;
        }
    }

    *p = s;
    return value * sign;
}

/**
 * @brief Fast ASCII string to 64-bit signed integer.
 * 
 * Advances the given pointer to string.
 * 
 * @param p Pointer to string (char array).
 * @return int64_t 
 */
static inline int64_t fast_str2i64(const char **p) {
    const char *s = *p;

    int64_t sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    }

    int64_t value = 0;
    while (*s >= '0' && *s <= '9') {
        // ASCII char - ASCII char = integer value of the char
        value = value * 10 + (*s - '0');
        s++;
    }

    *p = s;
    return value * sign;
}


static inline bool is_whitespace(char chr) {
    return (chr == ' ' || chr == '\t' || chr == '\r' || chr == '\n');
}

#define ADVANCE obj->current++

static inline void skip_whitespace(lvOBJ *obj) {
    while (*obj->current != '\0' && is_whitespace(*obj->current)) {
        ADVANCE;
    }
}

static inline void skip_line(lvOBJ *obj) {
    while (*obj->current != '\0') {
        ADVANCE;
        if (*obj->current == '\n') {
            break;
        }
    }

    // Skip \n
    ADVANCE;
}

static inline void parse_vertex(lvOBJ *obj) {
    /*
        Syntax:
        v float float float
    */

    ADVANCE; // skip v

    skip_whitespace(obj);
    float x = fast_str2f(&obj->current);

    skip_whitespace(obj);
    float y = fast_str2f(&obj->current);

    skip_whitespace(obj);
    float z = fast_str2f(&obj->current);

    lvOBJVec3 vertex = {x, y, z};
    lvArray_add(&obj->vertices, &vertex);
}

static inline void parse_normal(lvOBJ *obj) {
    /*
        Syntax:
        vn float float float
    */

    ADVANCE; ADVANCE; // skip vn

    skip_whitespace(obj);
    float x = fast_str2f(&obj->current);

    skip_whitespace(obj);
    float y = fast_str2f(&obj->current);

    skip_whitespace(obj);
    float z = fast_str2f(&obj->current);

    lvOBJVec3 normal = {x, y, z};
    lvArray_add(&obj->normals, &normal);
}

static inline void parse_uv(lvOBJ *obj) {
    /*
        Syntax:
        vt float float
    */

    ADVANCE; ADVANCE; // skip vt

    skip_whitespace(obj);
    float x = fast_str2f(&obj->current);

    skip_whitespace(obj);
    float y = fast_str2f(&obj->current);

    lvOBJVec2 uv = {x, y};
    lvArray_add(&obj->uvs, &uv);
}

static inline void parse_face(lvOBJ *obj) {
    /*
        Syntax:
        f int/[int]/[int] int/[int]/[int] int/[int]/[int]
    */

    // TODO: optional normals & uvs

    ADVANCE; // skip f

    skip_whitespace(obj);

    int64_t v0 = fast_str2i64(&obj->current);

    ADVANCE;
    int64_t uv0 = fast_str2i64(&obj->current);

    ADVANCE;
    int64_t n0 = fast_str2i64(&obj->current);

    skip_whitespace(obj);

    int64_t v1 = fast_str2i64(&obj->current);

    ADVANCE;
    int64_t uv1 = fast_str2i64(&obj->current);

    ADVANCE;
    int64_t n1 = fast_str2i64(&obj->current);

    skip_whitespace(obj);

    int64_t v2 = fast_str2i64(&obj->current);

    ADVANCE;
    int64_t uv2 = fast_str2i64(&obj->current);

    ADVANCE;
    int64_t n2 = fast_str2i64(&obj->current);

    lvOBJFace face = {
        .vertex_ids = {v0, v1, v2},
        .normal_ids = {n0, n1, n2},
        .uv_ids = {uv0, uv1, uv2}
    };
    lvArray_add(&obj->faces, &face);
}

static void parse_obj(lvOBJ *obj) {
    while (*obj->current != '\0') {
        skip_whitespace(obj);

        if (*obj->current == '#') {
            skip_line(obj);
        }

        else if (*obj->current == 'v' && is_whitespace(*(obj->current + 1))) {
            parse_vertex(obj);
        }

        else if (*obj->current == 'v' && *(obj->current + 1) == 'n') {
            parse_normal(obj);
        }

        else if (*obj->current == 'v' && *(obj->current + 1) == 't') {
            parse_uv(obj);
        }

        else if (*obj->current == 'f') {
            parse_face(obj);
        }
        
        else {
            skip_line(obj);
        }
    }
}

static inline void parse_newmtl(lvOBJ *obj) {
    /*
        Syntax:
        newmtl string
    */

    ADVANCE; ADVANCE; ADVANCE; ADVANCE; ADVANCE; ADVANCE; // skip newmtl

    skip_whitespace(obj);

    char name[LV_OBJ_MTL_NAME_LENGTH];
    size_t i = 0;
    while (
        *obj->current != '\0' &&
        !is_whitespace(*obj->current) &&
        i < LV_OBJ_MTL_NAME_LENGTH
    ) {
        name[i] = *obj->current;

        ADVANCE;
        i++;
    }

    lvOBJMaterialPBR mat = {0};
    memcpy(mat.name, name, LV_OBJ_MTL_NAME_LENGTH);
    memcpy(obj->current_mtl_name, name, LV_OBJ_MTL_NAME_LENGTH);

    lvArray_add(&obj->materials, &mat);
}

static inline void parse_roughness(lvOBJ *obj) {
    /*
        Syntax:
        Pr float
    */

    ADVANCE; ADVANCE; // skip Pr

    skip_whitespace(obj);

    float x = fast_str2f(&obj->current);

    printf("roughness: %f\n", x);

    lvOBJMaterialPBR *mat = lvOBJ_get_material(obj, obj->current_mtl_name);
    mat->roughness = x;
}

static void parse_mtl(lvOBJ *obj) {
    while (*obj->current != '\0') {
        skip_whitespace(obj);

        if (*obj->current == '#') {
            skip_line(obj);
        }

        else if (
            *(obj->current + 0) == 'n' &&
            *(obj->current + 1) == 'e' &&
            *(obj->current + 2) == 'w' &&
            *(obj->current + 3) == 'm' &&
            *(obj->current + 4) == 't' &&
            *(obj->current + 5) == 'l'
        ) {
            parse_newmtl(obj);
        }

        else if (
            *(obj->current + 0) == 'P' &&
            *(obj->current + 1) == 'r'
        ) {
            parse_roughness(obj);
        }
        
        else {
            skip_line(obj);
        }
    }
}


lvOBJ lvOBJ_load_raw(char *source) {
    lvOBJ obj;

    obj.vertices = lvArray_new(sizeof(vec3));
    obj.normals = lvArray_new(sizeof(vec3));
    obj.uvs = lvArray_new(sizeof(vec2));
    obj.faces = lvArray_new(sizeof(lvOBJFace));

    obj.mesh.tris = lvArray_new(sizeof(lvOBJTri));

    obj.current = source;

    parse_obj(&obj);

    for (size_t i = 0; i < obj.faces.size; i++) {
        lvOBJFace *face = LV_ARRAY_PTR_AT(&obj.faces, i, lvOBJFace);

        lvOBJTri tri = {
            .vertices = {
                LV_ARRAY_AT(&obj.vertices, face->vertex_ids[0] - 1, lvOBJVec3),
                LV_ARRAY_AT(&obj.vertices, face->vertex_ids[1] - 1, lvOBJVec3),
                LV_ARRAY_AT(&obj.vertices, face->vertex_ids[2] - 1, lvOBJVec3)
            },
            .normals = {
                LV_ARRAY_AT(&obj.normals, face->normal_ids[0] - 1, lvOBJVec3),
                LV_ARRAY_AT(&obj.normals, face->normal_ids[1] - 1, lvOBJVec3),
                LV_ARRAY_AT(&obj.normals, face->normal_ids[2] - 1, lvOBJVec3)
            },
            .uvs = {
                LV_ARRAY_AT(&obj.uvs, face->uv_ids[0] - 1, lvOBJVec2),
                LV_ARRAY_AT(&obj.uvs, face->uv_ids[1] - 1, lvOBJVec2),
                LV_ARRAY_AT(&obj.uvs, face->uv_ids[2] - 1, lvOBJVec2)
            },
        };

        lvArray_add(&obj.mesh.tris, &tri);
    }

    lvArray_free(&obj.vertices);
    lvArray_free(&obj.normals);
    lvArray_free(&obj.uvs);
    lvArray_free(&obj.faces);

    return obj;
}

lvOBJ lvOBJ_load(const char *filepath) {
    lvFileContent content = lv_read_file_raw(filepath);
    if (!content.data) {
        return (lvOBJ){0};
    }

    lvOBJ obj = lvOBJ_load_raw(content.data);
    obj.loaded = true;
    obj.materials = lvArray_new(sizeof(lvOBJMaterialPBR));

    LV_FREE(content.data);
    
    return obj;
}

lvOBJ lvOBJ_load_with_mtl(const char *obj_filepath, const char *mtl_filepath) {
    lvOBJ obj = lvOBJ_load(obj_filepath);
    
    lvFileContent mtl_content = lv_read_file_raw(mtl_filepath);
    if (!mtl_content.data) {
        obj.loaded = false;
        return obj;
    }

    obj.current = mtl_content.data;
    parse_mtl(&obj);

    LV_FREE(mtl_content.data);

    return obj;
}

void lvOBJ_free(lvOBJ *obj) {
    if (!obj) return;

    lvArray_free(&obj->mesh.tris);
    lvArray_free(&obj->materials);
}

lvOBJMaterialPBR *lvOBJ_get_material(lvOBJ *obj, const char *name) {
    if (!obj->loaded || !lvArray_valid(&obj->materials)) {
        return NULL;
    }

    bool found = false;
    lvOBJMaterialPBR *found_mat;
    for (size_t i = 0; i < obj->materials.size; i++) {
        lvOBJMaterialPBR *mat = LV_ARRAY_PTR_AT(&obj->materials, i, lvOBJMaterialPBR);

        if (strcmp(mat->name, name) == 0) {
            found = true;
            found_mat = mat;
            break;
        }
    }

    if (found) {
        return found_mat;
    }

    return NULL;
}