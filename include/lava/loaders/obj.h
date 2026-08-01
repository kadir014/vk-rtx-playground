#ifndef LV_LOADER_WAVEFRONT_OBJ_H
#define LV_LOADER_WAVEFRONT_OBJ_H

#include "lava/internal.h"
#include "lava/containers/array.h"


/**
 * Wavefront OBJ (.obj) files are the simplest type to store 3D scenes, geometry
 * and materials. It comes with limitations, but perfect for basic usecases.
 *
 * Follows the specification in https://en.wikipedia.org/wiki/Wavefront_.obj_file
 */


// TODO: Use glm's vectors

typedef struct {
    float x;
    float y;
    float z;
} lvOBJVec3;

typedef struct {
    float x;
    float y;
} lvOBJVec2;


/**
 * @brief Face definition with property IDs in OBJ file.
 * 
 * f v/n/uv [...]
 */
typedef struct {
    long vertex_ids[3];
    long normal_ids[3];
    long uv_ids[3];
} lvOBJFace;

/**
 * @brief Simple triangle face.
 */
typedef struct {
    lvOBJVec3 vertices[3];
    lvOBJVec3 normals[3];
    lvOBJVec2 uvs[3];
} lvOBJTri;

/**
 * @brief Triangular mesh geometry defined in OBJ file.
 */
typedef struct {
    lvArray tris;
} lvOBJMesh;

/**
 * @brief Type representing loaded Wavefront OBJ file.
 */
typedef struct {
    /*
        Temporary state used by the loader.
    */
    char *current;
    lvArray vertices;
    lvArray normals;
    lvArray uvs;
    lvArray faces;
    
    /*
        Public members.
    */
    bool loaded;
    lvOBJMesh mesh;
} lvOBJ;

/**
 * @brief Load OBJ from source null-terminated string.
 * 
 * @param source OBJ content.
 * @return lvOBJ 
 */
lvOBJ lvOBJ_load_raw(char *source);

/**
 * @brief Load OBJ from file.
 * 
 * @param filepath Path to Wavefront OBJ file.
 * @return lvOBJ 
 */
lvOBJ lvOBJ_load(const char *filepath);


#endif // LV_LOADER_WAVEFRONT_OBJ_H