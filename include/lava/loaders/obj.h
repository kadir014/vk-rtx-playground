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


#define LV_OBJ_MTL_NAME_LENGTH 64
#define LV_OBJ_MTL_MAP_FILEPATH_LENGTH 256


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
 * Currently only supports triangulated faces.
 * 
 * f v/n/uv [...]
 */
typedef struct {
    int64_t vertex_ids[3];
    int64_t normal_ids[3];
    int64_t uv_ids[3];
} lvOBJFace;

/**
 * @brief Single triangular face.
 */
typedef struct {
    lvOBJVec3 vertices[3];
    lvOBJVec3 normals[3];
    lvOBJVec2 uvs[3];
} lvOBJTri;

/**
 * @brief Triangular mesh geometry.
 */
typedef struct {
    char material_name[LV_OBJ_MTL_NAME_LENGTH];
    lvArray tris;
} lvOBJMesh;

/**
 * @brief Type representing a loaded Wavefront OBJ file.
 */
typedef struct {
    /*
        Temporary state used by the loader.
    */
    char *current;
    char current_mtl_name[LV_OBJ_MTL_NAME_LENGTH];
    lvArray vertices;
    lvArray normals;
    lvArray uvs;
    lvArray faces;
    
    /*
        Public members.
    */
    bool loaded;
    lvArray materials;
    lvOBJMesh mesh;
} lvOBJ;

/**
 * @brief Material definition in a MTL file using extended PBR properties.
 */
typedef struct {
    char name[LV_OBJ_MTL_NAME_LENGTH];

    float metallic;
    char metallic_map[LV_OBJ_MTL_MAP_FILEPATH_LENGTH];

    float roughness;
    char roughness_map[LV_OBJ_MTL_MAP_FILEPATH_LENGTH];

    float sheen;
    char sheen_map[LV_OBJ_MTL_MAP_FILEPATH_LENGTH];

    float clearcoat_thickness;
    float clearcoat_roughness;

    lvOBJVec3 emissive;
    char emissive_map[LV_OBJ_MTL_MAP_FILEPATH_LENGTH];

    float anisotropy;
    float anisotropy_rotation;

    char normal_map[LV_OBJ_MTL_MAP_FILEPATH_LENGTH];

    float ior;
} lvOBJMaterialPBR;

/**
 * @brief Load Wavefornt OBJ from source null-terminated string.
 * 
 * Use `lvObj.loaded` member to see if initialization was successful before using.
 * 
 * @param source Null-terminated Wavefront OBJ content.
 * @return lvOBJ 
 */
lvOBJ lvOBJ_load_raw(char *source);

/**
 * @brief Load Wavefront OBJ from file.
 * 
 * Use `lvObj.loaded` member to see if initialization was successful before using.
 * 
 * @param filepath Path to Wavefront OBJ file.
 * @return lvOBJ 
 */
lvOBJ lvOBJ_load(const char *filepath);

/**
 * @brief Load Wavefront OBJ from file with its material libary.
 * 
 * Use `lvObj.loaded` member to see if initialization was successful before using.
 * 
 * @param obj_filepath Path to Wavefront OBJ file.
 * @param mtl_filepath Path to material library file.
 * @return lvOBJ 
 */
lvOBJ lvOBJ_load_with_mtl(const char *obj_filepath, const char *mtl_filepath);

/**
 * @brief Free OBJ instance.
 * 
 * It's safe to pass `NULL` to this function.
 * 
 * @param obj OBJ instance to free.
 */
void lvOBJ_free(lvOBJ *obj);

/**
 * @brief Get loaded material with given name.
 * 
 * Returns `NULL` if failed to fetch.
 * 
 * @param obj Loaded OBJ instance.
 * @param name Material name defined in MTL file.
 * @return lvOBJMaterialPBR *
 */
lvOBJMaterialPBR *lvOBJ_get_material(lvOBJ *obj, const char *name);


#endif // LV_LOADER_WAVEFRONT_OBJ_H