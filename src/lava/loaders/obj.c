#include "lava/loaders/obj.h"
#include "lava/core/io.h"


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


static inline float fast_atof(const char **p) {
    const char *s = *p;
    float sign = 1.0f;
    if (*s == '-') { sign = -1.0f; s++; }
    else if (*s == '+') { s++; }

    float value = 0.0f;
    while (*s >= '0' && *s <= '9') {
        value = value * 10.0f + (*s - '0');
        s++;
    }

    if (*s == '.') {
        s++;
        float frac = 0.0f;
        float scale = 1.0f;
        while (*s >= '0' && *s <= '9') {
            frac = frac * 10.0f + (*s - '0');
            scale *= 10.0f;
            s++;
        }
        value += frac / scale;
    }

    *p = s;
    return value * sign;
}

static inline int fast_atoi(const char **p) {
    const char *s = *p;
    int sign = 1;
    if (*s == '-') { sign = -1; s++; }

    int value = 0;
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    *p = s;
    return value * sign;
}

static inline float parse_float(lvOBJ *obj) {
    char *end;
    float value = strtof(obj->current, &end);
    obj->current = end;
    return value;

    //return fast_atof(&obj->current);
}

static inline long parse_long(lvOBJ *obj) {
    char *end;
    long value = strtol(obj->current, &end, 10);
    obj->current = end;
    return value;
    //return (long)fast_atoi(&obj->current);
}

static inline void parse_vertex(lvOBJ *obj) {
    /*
        Syntax:
        v float float float
    */

    ADVANCE; // skip v

    skip_whitespace(obj);
    float x = parse_float(obj);

    skip_whitespace(obj);
    float y = parse_float(obj);

    skip_whitespace(obj);
    float z = parse_float(obj);

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
    float x = parse_float(obj);

    skip_whitespace(obj);
    float y = parse_float(obj);

    skip_whitespace(obj);
    float z = parse_float(obj);

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
    float x = parse_float(obj);

    skip_whitespace(obj);
    float y = parse_float(obj);

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

    long v0 = parse_long(obj);

    ADVANCE;
    long uv0 = parse_long(obj);

    ADVANCE;
    long n0 = parse_long(obj);

    skip_whitespace(obj);

    long v1 = parse_long(obj);

    ADVANCE;
    long uv1 = parse_long(obj);

    ADVANCE;
    long n1 = parse_long(obj);

    skip_whitespace(obj);

    long v2 = parse_long(obj);

    ADVANCE;
    long uv2 = parse_long(obj);

    ADVANCE;
    long n2 = parse_long(obj);

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

    LV_FREE(content.data);
    
    return obj;
}