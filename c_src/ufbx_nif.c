/* SPDX-License-Identifier: MIT
 * Copyright (c) 2025-present K. S. Ernest (iFire) Lee
 */

#include <erl_nif.h>
#include <string.h>
#include "ufbx.h"
#include "ufbx_write.h"

// Helper: Convert ufbx_vec3 to Elixir list [x, y, z]
static ERL_NIF_TERM make_vec3(ErlNifEnv* env, ufbx_vec3 vec) {
    ERL_NIF_TERM x = enif_make_double(env, vec.x);
    ERL_NIF_TERM y = enif_make_double(env, vec.y);
    ERL_NIF_TERM z = enif_make_double(env, vec.z);
    return enif_make_list3(env, x, y, z);
}

// Helper: Convert ufbx_vec4/quat to Elixir list [x, y, z, w]
static ERL_NIF_TERM make_vec4(ErlNifEnv* env, ufbx_vec4 vec) {
    ERL_NIF_TERM x = enif_make_double(env, vec.x);
    ERL_NIF_TERM y = enif_make_double(env, vec.y);
    ERL_NIF_TERM z = enif_make_double(env, vec.z);
    ERL_NIF_TERM w = enif_make_double(env, vec.w);
    return enif_make_list4(env, x, y, z, w);
}

// Helper: Convert ufbx_string to Elixir binary
static ERL_NIF_TERM make_string(ErlNifEnv* env, ufbx_string str) {
    if (str.length == 0) {
        return enif_make_string(env, "", ERL_NIF_LATIN1);
    }
    // Allocate binary and copy string data
    ErlNifBinary bin;
    if (!enif_alloc_binary(str.length, &bin)) {
        return enif_make_atom(env, "error");
    }
    memcpy(bin.data, str.data, str.length);
    ERL_NIF_TERM result = enif_make_binary(env, &bin);
    return result;
}

// Helper: Convert ufbx_vec3_list to Elixir list
static ERL_NIF_TERM make_vec3_list(ErlNifEnv* env, ufbx_vec3_list list) {
    ERL_NIF_TERM result = enif_make_list(env, 0);
    for (size_t i = list.count; i > 0; i--) {
        ERL_NIF_TERM vec = make_vec3(env, list.data[i - 1]);
        result = enif_make_list_cell(env, vec, result);
    }
    return result;
}

// Helper: Convert ufbx_uint32_list to Elixir list
static ERL_NIF_TERM make_uint32_list(ErlNifEnv* env, ufbx_uint32_list list) {
    ERL_NIF_TERM result = enif_make_list(env, 0);
    for (size_t i = list.count; i > 0; i--) {
        ERL_NIF_TERM val = enif_make_uint(env, list.data[i - 1]);
        result = enif_make_list_cell(env, val, result);
    }
    return result;
}

// Extract node data from ufbx_node to Elixir map
static ERL_NIF_TERM extract_node(ErlNifEnv* env, ufbx_node *node, uint32_t node_id) {
    ERL_NIF_TERM keys[10];
    ERL_NIF_TERM values[10];
    size_t idx = 0;
    
    // id
    keys[idx] = enif_make_atom(env, "id");
    values[idx] = enif_make_uint(env, node_id);
    idx++;
    
    // name
    keys[idx] = enif_make_atom(env, "name");
    values[idx] = make_string(env, node->name);
    idx++;
    
    // parent_id (if parent exists, use parent's typed_id)
    if (node->parent) {
        keys[idx] = enif_make_atom(env, "parent_id");
        values[idx] = enif_make_uint(env, node->parent->typed_id);
        idx++;
    }
    
    // children (list of typed_ids)
    if (node->children.count > 0) {
        ERL_NIF_TERM children = enif_make_list(env, 0);
        for (size_t i = node->children.count; i > 0; i--) {
            ERL_NIF_TERM child_id = enif_make_uint(env, node->children.data[i - 1]->typed_id);
            children = enif_make_list_cell(env, child_id, children);
        }
        keys[idx] = enif_make_atom(env, "children");
        values[idx] = children;
        idx++;
    }
    
    // translation from local_transform
    keys[idx] = enif_make_atom(env, "translation");
    values[idx] = make_vec3(env, node->local_transform.translation);
    idx++;
    
    // rotation from local_transform (quaternion)
    keys[idx] = enif_make_atom(env, "rotation");
    ufbx_vec4 rot_vec4;
    rot_vec4.x = node->local_transform.rotation.x;
    rot_vec4.y = node->local_transform.rotation.y;
    rot_vec4.z = node->local_transform.rotation.z;
    rot_vec4.w = node->local_transform.rotation.w;
    values[idx] = make_vec4(env, rot_vec4);
    idx++;
    
    // scale from local_transform
    keys[idx] = enif_make_atom(env, "scale");
    values[idx] = make_vec3(env, node->local_transform.scale);
    idx++;
    
    // mesh_id (if mesh exists)
    if (node->mesh) {
        keys[idx] = enif_make_atom(env, "mesh_id");
        values[idx] = enif_make_uint(env, node->mesh->typed_id);
        idx++;
    }
    
    // Build map manually for compatibility
    ERL_NIF_TERM map = enif_make_new_map(env);
    for (size_t i = 0; i < idx; i++) {
        enif_make_map_put(env, map, keys[i], values[i], &map);
    }
    return map;
}

// Extract mesh data from ufbx_mesh to Elixir map
static ERL_NIF_TERM extract_mesh(ErlNifEnv* env, ufbx_mesh *mesh) {
    ERL_NIF_TERM keys[10];
    ERL_NIF_TERM values[10];
    size_t idx = 0;
    
    // id
    keys[idx] = enif_make_atom(env, "id");
    values[idx] = enif_make_uint(env, mesh->typed_id);
    idx++;
    
    // name
    keys[idx] = enif_make_atom(env, "name");
    values[idx] = make_string(env, mesh->name);
    idx++;
    
    // positions (from vertex_position)
    if (mesh->vertex_position.exists && mesh->vertex_position.values.count > 0) {
        ERL_NIF_TERM positions = make_vec3_list(env, mesh->vertex_position.values);
        keys[idx] = enif_make_atom(env, "positions");
        values[idx] = positions;
        idx++;
        
        // indices (from vertex_position.indices)
        if (mesh->vertex_position.indices.count > 0) {
            ERL_NIF_TERM indices = make_uint32_list(env, mesh->vertex_position.indices);
            keys[idx] = enif_make_atom(env, "indices");
            values[idx] = indices;
            idx++;
        }
    }
    
    // normals (from vertex_normal)
    if (mesh->vertex_normal.exists && mesh->vertex_normal.values.count > 0) {
        ERL_NIF_TERM normals = make_vec3_list(env, mesh->vertex_normal.values);
        keys[idx] = enif_make_atom(env, "normals");
        values[idx] = normals;
        idx++;
    }
    
    // texcoords (from vertex_uv)
    if (mesh->vertex_uv.exists && mesh->vertex_uv.values.count > 0) {
        ERL_NIF_TERM texcoords = enif_make_list(env, 0);
        for (size_t i = mesh->vertex_uv.values.count; i > 0; i--) {
            ufbx_vec2 uv = mesh->vertex_uv.values.data[i - 1];
            ERL_NIF_TERM u = enif_make_double(env, uv.x);
            ERL_NIF_TERM v = enif_make_double(env, uv.y);
            ERL_NIF_TERM uv_vec = enif_make_list2(env, u, v);
            texcoords = enif_make_list_cell(env, uv_vec, texcoords);
        }
        keys[idx] = enif_make_atom(env, "texcoords");
        values[idx] = texcoords;
        idx++;
    }
    
    // material_ids
    if (mesh->materials.count > 0) {
        ERL_NIF_TERM material_ids = enif_make_list(env, 0);
        for (size_t i = mesh->materials.count; i > 0; i--) {
            ERL_NIF_TERM mat_id = enif_make_uint(env, mesh->materials.data[i - 1]->typed_id);
            material_ids = enif_make_list_cell(env, mat_id, material_ids);
        }
        keys[idx] = enif_make_atom(env, "material_ids");
        values[idx] = material_ids;
        idx++;
    }
    
    // Build map manually for compatibility
    ERL_NIF_TERM map = enif_make_new_map(env);
    for (size_t i = 0; i < idx; i++) {
        enif_make_map_put(env, map, keys[i], values[i], &map);
    }
    return map;
}

// Extract material data from ufbx_material to Elixir map
static ERL_NIF_TERM extract_material(ErlNifEnv* env, ufbx_material *material) {
    ERL_NIF_TERM keys[10];
    ERL_NIF_TERM values[10];
    size_t idx = 0;
    
    // id
    keys[idx] = enif_make_atom(env, "id");
    values[idx] = enif_make_uint(env, material->typed_id);
    idx++;
    
    // name
    keys[idx] = enif_make_atom(env, "name");
    values[idx] = make_string(env, material->name);
    idx++;
    
    // diffuse_color (from PBR base_color or FBX diffuse)
    if (material->pbr.base_color.has_value && material->pbr.base_color.value_components >= 3) {
        ufbx_vec3 color = material->pbr.base_color.value_vec3;
        keys[idx] = enif_make_atom(env, "diffuse_color");
        values[idx] = make_vec3(env, color);
        idx++;
    } else if (material->fbx.diffuse_color.has_value && material->fbx.diffuse_color.value_components >= 3) {
        ufbx_vec3 color = material->fbx.diffuse_color.value_vec3;
        keys[idx] = enif_make_atom(env, "diffuse_color");
        values[idx] = make_vec3(env, color);
        idx++;
    }
    
    // specular_color (from PBR specular_color or FBX)
    if (material->pbr.specular_color.has_value && material->pbr.specular_color.value_components >= 3) {
        ufbx_vec3 color = material->pbr.specular_color.value_vec3;
        keys[idx] = enif_make_atom(env, "specular_color");
        values[idx] = make_vec3(env, color);
        idx++;
    } else if (material->fbx.specular_color.has_value && material->fbx.specular_color.value_components >= 3) {
        ufbx_vec3 color = material->fbx.specular_color.value_vec3;
        keys[idx] = enif_make_atom(env, "specular_color");
        values[idx] = make_vec3(env, color);
        idx++;
    }
    
    // emissive_color (from PBR emission_color or FBX)
    if (material->pbr.emission_color.has_value && material->pbr.emission_color.value_components >= 3) {
        ufbx_vec3 color = material->pbr.emission_color.value_vec3;
        keys[idx] = enif_make_atom(env, "emissive_color");
        values[idx] = make_vec3(env, color);
        idx++;
    } else if (material->fbx.emission_color.has_value && material->fbx.emission_color.value_components >= 3) {
        ufbx_vec3 color = material->fbx.emission_color.value_vec3;
        keys[idx] = enif_make_atom(env, "emissive_color");
        values[idx] = make_vec3(env, color);
        idx++;
    }
    
    // Build map manually for compatibility
    ERL_NIF_TERM map = enif_make_new_map(env);
    for (size_t i = 0; i < idx; i++) {
        enif_make_map_put(env, map, keys[i], values[i], &map);
    }
    return map;
}

// Extract texture data from ufbx_texture to Elixir map
static ERL_NIF_TERM extract_texture(ErlNifEnv* env, ufbx_texture *texture) {
    ERL_NIF_TERM keys[5];
    ERL_NIF_TERM values[5];
    size_t idx = 0;
    
    // id
    keys[idx] = enif_make_atom(env, "id");
    values[idx] = enif_make_uint(env, texture->typed_id);
    idx++;
    
    // name
    keys[idx] = enif_make_atom(env, "name");
    values[idx] = make_string(env, texture->name);
    idx++;
    
    // file_path (from filename)
    if (texture->filename.length > 0) {
        keys[idx] = enif_make_atom(env, "file_path");
        values[idx] = make_string(env, texture->filename);
        idx++;
    }
    
    // Build map manually for compatibility
    ERL_NIF_TERM map = enif_make_new_map(env);
    for (size_t i = 0; i < idx; i++) {
        enif_make_map_put(env, map, keys[i], values[i], &map);
    }
    return map;
}

// Extract keyframe from ufbx_baked_vec3 to Elixir map (translation/scale)
static ERL_NIF_TERM extract_vec3_keyframe(ErlNifEnv* env, ufbx_baked_vec3 *key, const char* field_name) {
    ERL_NIF_TERM keys[2];
    ERL_NIF_TERM values[2];
    
    keys[0] = enif_make_atom(env, "time");
    values[0] = enif_make_double(env, key->time);
    
    keys[1] = enif_make_atom(env, field_name);
    values[1] = make_vec3(env, key->value);
    
    ERL_NIF_TERM map = enif_make_new_map(env);
    enif_make_map_put(env, map, keys[0], values[0], &map);
    enif_make_map_put(env, map, keys[1], values[1], &map);
    
    return map;
}

// Extract keyframe from ufbx_baked_quat to Elixir map (rotation)
static ERL_NIF_TERM extract_quat_keyframe(ErlNifEnv* env, ufbx_baked_quat *key) {
    ERL_NIF_TERM keys[2];
    ERL_NIF_TERM values[2];
    
    keys[0] = enif_make_atom(env, "time");
    values[0] = enif_make_double(env, key->time);
    
    // Convert ufbx_quat to vec4 format [x, y, z, w]
    keys[1] = enif_make_atom(env, "rotation");
    ERL_NIF_TERM x = enif_make_double(env, key->value.x);
    ERL_NIF_TERM y = enif_make_double(env, key->value.y);
    ERL_NIF_TERM z = enif_make_double(env, key->value.z);
    ERL_NIF_TERM w = enif_make_double(env, key->value.w);
    values[1] = enif_make_list4(env, x, y, z, w);
    
    ERL_NIF_TERM map = enif_make_new_map(env);
    enif_make_map_put(env, map, keys[0], values[0], &map);
    enif_make_map_put(env, map, keys[1], values[1], &map);
    
    return map;
}

// Extract animation data from ufbx_baked_anim to Elixir map
static ERL_NIF_TERM extract_animation(ErlNifEnv* env, ufbx_baked_anim *baked, ufbx_anim_stack *anim_stack) {
    ERL_NIF_TERM keys[4];
    ERL_NIF_TERM values[4];
    size_t idx = 0;
    
    // id (use anim_stack typed_id)
    keys[idx] = enif_make_atom(env, "id");
    values[idx] = enif_make_uint(env, anim_stack->typed_id);
    idx++;
    
    // name
    keys[idx] = enif_make_atom(env, "name");
    values[idx] = make_string(env, anim_stack->name);
    idx++;
    
    // Extract keyframes per node
    ERL_NIF_TERM all_keyframes = enif_make_list(env, 0);
    
    for (size_t i = baked->nodes.count; i > 0; i--) {
        ufbx_baked_node *baked_node = &baked->nodes.data[i - 1];
        uint32_t node_id = baked_node->typed_id;
        
        // Extract translation keyframes
        for (size_t j = baked_node->translation_keys.count; j > 0; j--) {
            ufbx_baked_vec3 *trans_key = &baked_node->translation_keys.data[j - 1];
            ERL_NIF_TERM keyframe = extract_vec3_keyframe(env, trans_key, "translation");
            
            // Add node_id to keyframe
            ERL_NIF_TERM keyframe_map = enif_make_new_map(env);
            ERL_NIF_TERM node_id_key = enif_make_atom(env, "node_id");
            ERL_NIF_TERM node_id_val = enif_make_uint(env, node_id);
            enif_make_map_put(env, keyframe_map, node_id_key, node_id_val, &keyframe_map);
            
            // Copy keyframe fields into map
            ERL_NIF_TERM value;
            if (enif_get_map_value(env, keyframe, enif_make_atom(env, "time"), &value)) {
                enif_make_map_put(env, keyframe_map, enif_make_atom(env, "time"), value, &keyframe_map);
            }
            if (enif_get_map_value(env, keyframe, enif_make_atom(env, "translation"), &value)) {
                enif_make_map_put(env, keyframe_map, enif_make_atom(env, "translation"), value, &keyframe_map);
            }
            
            all_keyframes = enif_make_list_cell(env, keyframe_map, all_keyframes);
        }
        
        // Extract rotation keyframes
        for (size_t j = baked_node->rotation_keys.count; j > 0; j--) {
            ufbx_baked_quat *rot_key = &baked_node->rotation_keys.data[j - 1];
            ERL_NIF_TERM keyframe = extract_quat_keyframe(env, rot_key);
            
            // Add node_id to keyframe
            ERL_NIF_TERM keyframe_map = enif_make_new_map(env);
            ERL_NIF_TERM node_id_key = enif_make_atom(env, "node_id");
            ERL_NIF_TERM node_id_val = enif_make_uint(env, node_id);
            enif_make_map_put(env, keyframe_map, node_id_key, node_id_val, &keyframe_map);
            
            // Copy keyframe fields into map
            ERL_NIF_TERM value;
            if (enif_get_map_value(env, keyframe, enif_make_atom(env, "time"), &value)) {
                enif_make_map_put(env, keyframe_map, enif_make_atom(env, "time"), value, &keyframe_map);
            }
            if (enif_get_map_value(env, keyframe, enif_make_atom(env, "rotation"), &value)) {
                enif_make_map_put(env, keyframe_map, enif_make_atom(env, "rotation"), value, &keyframe_map);
            }
            
            all_keyframes = enif_make_list_cell(env, keyframe_map, all_keyframes);
        }
        
        // Extract scale keyframes
        for (size_t j = baked_node->scale_keys.count; j > 0; j--) {
            ufbx_baked_vec3 *scale_key = &baked_node->scale_keys.data[j - 1];
            ERL_NIF_TERM keyframe = extract_vec3_keyframe(env, scale_key, "scale");
            
            // Add node_id to keyframe
            ERL_NIF_TERM keyframe_map = enif_make_new_map(env);
            ERL_NIF_TERM node_id_key = enif_make_atom(env, "node_id");
            ERL_NIF_TERM node_id_val = enif_make_uint(env, node_id);
            enif_make_map_put(env, keyframe_map, node_id_key, node_id_val, &keyframe_map);
            
            // Copy keyframe fields into map
            ERL_NIF_TERM value;
            if (enif_get_map_value(env, keyframe, enif_make_atom(env, "time"), &value)) {
                enif_make_map_put(env, keyframe_map, enif_make_atom(env, "time"), value, &keyframe_map);
            }
            if (enif_get_map_value(env, keyframe, enif_make_atom(env, "scale"), &value)) {
                enif_make_map_put(env, keyframe_map, enif_make_atom(env, "scale"), value, &keyframe_map);
            }
            
            all_keyframes = enif_make_list_cell(env, keyframe_map, all_keyframes);
        }
    }
    
    // keyframes
    keys[idx] = enif_make_atom(env, "keyframes");
    values[idx] = all_keyframes;
    idx++;
    
    // Build map manually
    ERL_NIF_TERM map = enif_make_new_map(env);
    for (size_t i = 0; i < idx; i++) {
        enif_make_map_put(env, map, keys[i], values[i], &map);
    }
    
    return map;
}

// Helper: Extract scene data from ufbx_scene to Elixir map
static ERL_NIF_TERM extract_scene_data(ErlNifEnv* env, ufbx_scene *scene) {
    // Build nodes list
    ERL_NIF_TERM nodes = enif_make_list(env, 0);
    for (size_t i = scene->nodes.count; i > 0; i--) {
        ufbx_node *node = scene->nodes.data[i - 1];
        ERL_NIF_TERM node_term = extract_node(env, node, node->typed_id);
        nodes = enif_make_list_cell(env, node_term, nodes);
    }
    
    // Build meshes list
    ERL_NIF_TERM meshes = enif_make_list(env, 0);
    for (size_t i = scene->meshes.count; i > 0; i--) {
        ufbx_mesh *mesh = scene->meshes.data[i - 1];
        ERL_NIF_TERM mesh_term = extract_mesh(env, mesh);
        meshes = enif_make_list_cell(env, mesh_term, meshes);
    }
    
    // Build materials list
    ERL_NIF_TERM materials = enif_make_list(env, 0);
    for (size_t i = scene->materials.count; i > 0; i--) {
        ufbx_material *material = scene->materials.data[i - 1];
        ERL_NIF_TERM material_term = extract_material(env, material);
        materials = enif_make_list_cell(env, material_term, materials);
    }
    
    // Build textures list
    ERL_NIF_TERM textures = enif_make_list(env, 0);
    for (size_t i = scene->textures.count; i > 0; i--) {
        ufbx_texture *texture = scene->textures.data[i - 1];
        ERL_NIF_TERM texture_term = extract_texture(env, texture);
        textures = enif_make_list_cell(env, texture_term, textures);
    }
    
    // Extract animations from anim_stacks
    ERL_NIF_TERM animations = enif_make_list(env, 0);
    ufbx_bake_opts bake_opts = {0};
    bake_opts.resample_rate = 30.0; // 30 FPS default
    
    for (size_t i = scene->anim_stacks.count; i > 0; i--) {
        ufbx_anim_stack *anim_stack = scene->anim_stacks.data[i - 1];
        
        // Bake animation
        ufbx_error error;
        ufbx_baked_anim *baked = ufbx_bake_anim(scene, anim_stack->anim, &bake_opts, &error);
        
        if (baked) {
            ERL_NIF_TERM animation_term = extract_animation(env, baked, anim_stack);
            animations = enif_make_list_cell(env, animation_term, animations);
            ufbx_free_baked_anim(baked);
        }
    }
    
    // Build version string from metadata
    char version_str[32];
    snprintf(version_str, sizeof(version_str), "FBX %u.%u", 
             scene->metadata.version / 1000, 
             (scene->metadata.version % 1000) / 100);
    
    // Build result map
    ERL_NIF_TERM keys[6];
    ERL_NIF_TERM values[6];
    keys[0] = enif_make_atom(env, "version");
    values[0] = enif_make_string(env, version_str, ERL_NIF_LATIN1);
    keys[1] = enif_make_atom(env, "nodes");
    values[1] = nodes;
    keys[2] = enif_make_atom(env, "meshes");
    values[2] = meshes;
    keys[3] = enif_make_atom(env, "materials");
    values[3] = materials;
    keys[4] = enif_make_atom(env, "textures");
    values[4] = textures;
    keys[5] = enif_make_atom(env, "animations");
    values[5] = animations;
    
    // Build map manually for compatibility
    ERL_NIF_TERM scene_data = enif_make_new_map(env);
    for (size_t i = 0; i < 6; i++) {
        enif_make_map_put(env, scene_data, keys[i], values[i], &scene_data);
    }
    
    return scene_data;
}

static ERL_NIF_TERM load_fbx_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    (void)argc;  // Unused parameter
    ErlNifBinary file_path_bin;
    ufbx_error error;
    ufbx_scene *scene;
    
    // Get file path from Elixir
    if (!enif_inspect_binary(env, argv[0], &file_path_bin)) {
        return enif_make_badarg(env);
    }
    
    // Null-terminate the path
    char file_path[file_path_bin.size + 1];
    memcpy(file_path, file_path_bin.data, file_path_bin.size);
    file_path[file_path_bin.size] = '\0';
    
    // Load FBX file using ufbx
    ufbx_load_opts opts = { 0 };
    scene = ufbx_load_file(file_path, &opts, &error);
    
    if (!scene) {
        // Return error tuple
        return enif_make_tuple2(env,
            enif_make_atom(env, "error"),
            enif_make_string(env, error.description.data, ERL_NIF_LATIN1));
    }
    
    // Extract scene data
    ERL_NIF_TERM scene_data = extract_scene_data(env, scene);
    
    // Free scene
    ufbx_free_scene(scene);
    
    // Return ok tuple with scene data
    return enif_make_tuple2(env,
        enif_make_atom(env, "ok"),
        scene_data);
}

// Load FBX from binary data
static ERL_NIF_TERM load_fbx_binary_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    (void)argc;  // Unused parameter
    ErlNifBinary data_bin;
    ufbx_error error;
    ufbx_scene *scene;
    
    // Get binary data from Elixir
    if (!enif_inspect_binary(env, argv[0], &data_bin)) {
        return enif_make_badarg(env);
    }
    
    // Load FBX from memory using ufbx
    ufbx_load_opts opts = { 0 };
    scene = ufbx_load_memory(data_bin.data, data_bin.size, &opts, &error);
    
    if (!scene) {
        // Return error tuple
        return enif_make_tuple2(env,
            enif_make_atom(env, "error"),
            enif_make_string(env, error.description.data, ERL_NIF_LATIN1));
    }
    
    // Extract scene data
    ERL_NIF_TERM scene_data = extract_scene_data(env, scene);
    
    // Free scene
    ufbx_free_scene(scene);
    
    // Return ok tuple with scene data
    return enif_make_tuple2(env,
        enif_make_atom(env, "ok"),
        scene_data);
}

// ============================================================================
// Write NIF Functions (ufbx_write)
// ============================================================================

// Helper: Parse vec3 from Elixir list [x, y, z]
static int parse_vec3_from_list(ErlNifEnv* env, ERL_NIF_TERM list, ufbxw_vec3 *out) {
    unsigned int len;
    if (!enif_get_list_length(env, list, &len) || len != 3) {
        return 0;
    }
    
    ERL_NIF_TERM head;
    ERL_NIF_TERM tail = list;
    double values[3];
    
    for (int i = 0; i < 3; i++) {
        if (!enif_get_list_cell(env, tail, &head, &tail)) {
            return 0;
        }
        if (!enif_get_double(env, head, &values[i])) {
            return 0;
        }
    }
    
    out->x = values[0];
    out->y = values[1];
    out->z = values[2];
    return 1;
}

// Helper: Parse vec4/quat from Elixir list [x, y, z, w]
static int parse_vec4_from_list(ErlNifEnv* env, ERL_NIF_TERM list, ufbxw_vec4 *out) {
    unsigned int len;
    if (!enif_get_list_length(env, list, &len) || len != 4) {
        return 0;
    }
    
    ERL_NIF_TERM head;
    ERL_NIF_TERM tail = list;
    double values[4];
    
    for (int i = 0; i < 4; i++) {
        if (!enif_get_list_cell(env, tail, &head, &tail)) {
            return 0;
        }
        if (!enif_get_double(env, head, &values[i])) {
            return 0;
        }
    }
    
    out->x = values[0];
    out->y = values[1];
    out->z = values[2];
    out->w = values[3];
    return 1;
}

// Helper: Parse string from Elixir binary or atom
static int parse_string(ErlNifEnv* env, ERL_NIF_TERM term, char **out_str, size_t *out_len) {
    ErlNifBinary bin;
    if (enif_inspect_binary(env, term, &bin)) {
        *out_str = (char*)bin.data;
        *out_len = bin.size;
        return 1;
    }
    // Try atom
    unsigned int atom_len;
    if (enif_get_atom_length(env, term, &atom_len, ERL_NIF_LATIN1)) {
        char *atom_str = (char*)enif_alloc(atom_len + 1);
        if (enif_get_atom(env, term, atom_str, atom_len + 1, ERL_NIF_LATIN1)) {
            *out_str = atom_str;
            *out_len = atom_len;
            return 1;
        }
        enif_free(atom_str);
    }
    return 0;
}

// Helper: Get uint from map
static int get_map_uint(ErlNifEnv* env, ERL_NIF_TERM map, const char* key, unsigned int *out) {
    ERL_NIF_TERM value;
    ERL_NIF_TERM key_term = enif_make_atom(env, key);
    if (enif_get_map_value(env, map, key_term, &value)) {
        return enif_get_uint(env, value, out);
    }
    return 0;
}

// Helper: Get string from map
static int get_map_string(ErlNifEnv* env, ERL_NIF_TERM map, const char* key, char **out_str, size_t *out_len) {
    ERL_NIF_TERM value;
    ERL_NIF_TERM key_term = enif_make_atom(env, key);
    if (enif_get_map_value(env, map, key_term, &value)) {
        return parse_string(env, value, out_str, out_len);
    }
    return 0;
}

// Helper: Get vec3 from map
static int get_map_vec3(ErlNifEnv* env, ERL_NIF_TERM map, const char* key, ufbxw_vec3 *out) {
    ERL_NIF_TERM value;
    ERL_NIF_TERM key_term = enif_make_atom(env, key);
    if (enif_get_map_value(env, map, key_term, &value)) {
        return parse_vec3_from_list(env, value, out);
    }
    return 0;
}

// Helper: Get vec4 from map
static int get_map_vec4(ErlNifEnv* env, ERL_NIF_TERM map, const char* key, ufbxw_vec4 *out) {
    ERL_NIF_TERM value;
    ERL_NIF_TERM key_term = enif_make_atom(env, key);
    if (enif_get_map_value(env, map, key_term, &value)) {
        return parse_vec4_from_list(env, value, out);
    }
    return 0;
}

// Helper: Get list from map
static int get_map_list(ErlNifEnv* env, ERL_NIF_TERM map, const char* key, ERL_NIF_TERM *out) {
    ERL_NIF_TERM key_term = enif_make_atom(env, key);
    return enif_get_map_value(env, map, key_term, out);
}

// Build ufbxw_scene from Elixir map data
static ufbxw_scene* build_ufbxw_scene_from_map(ErlNifEnv* env, ERL_NIF_TERM scene_data_map) {
    ufbxw_scene_opts opts = {0};
    ufbxw_scene *scene = ufbxw_create_scene(&opts);
    if (!scene) {
        return NULL;
    }
    
    // Parse nodes
    ERL_NIF_TERM nodes_list;
    if (get_map_list(env, scene_data_map, "nodes", &nodes_list)) {
        unsigned int nodes_len;
        if (enif_get_list_length(env, nodes_list, &nodes_len)) {
            // Store node IDs for parent/child relationships
            ufbxw_node *node_handles = (ufbxw_node*)enif_alloc(sizeof(ufbxw_node) * nodes_len);
            unsigned int *node_ids = (unsigned int*)enif_alloc(sizeof(unsigned int) * nodes_len);
            
            ERL_NIF_TERM head, tail = nodes_list;
            for (unsigned int i = 0; i < nodes_len; i++) {
                if (!enif_get_list_cell(env, tail, &head, &tail)) break;
                
                // Create node
                ufbxw_node node = ufbxw_create_node(scene);
                node_handles[i] = node;
                
                // Get node ID
                unsigned int node_id;
                if (get_map_uint(env, head, "id", &node_id)) {
                    node_ids[i] = node_id;
                }
                
                // Set name
                char *name_str;
                size_t name_len;
                if (get_map_string(env, head, "name", &name_str, &name_len)) {
                    ufbxw_set_name_len(scene, node.id, name_str, name_len);
                }
                
                // Set translation
                ufbxw_vec3 translation = {0.0, 0.0, 0.0};
                if (get_map_vec3(env, head, "translation", &translation)) {
                    ufbxw_node_set_translation(scene, node, translation);
                }
                
                // Set rotation (as quaternion)
                ufbxw_vec4 rotation_vec4;
                if (get_map_vec4(env, head, "rotation", &rotation_vec4)) {
                    ufbxw_quat rotation_quat;
                    rotation_quat.x = rotation_vec4.x;
                    rotation_quat.y = rotation_vec4.y;
                    rotation_quat.z = rotation_vec4.z;
                    rotation_quat.w = rotation_vec4.w;
                    ufbxw_node_set_rotation_quat(scene, node, rotation_quat, UFBXW_ROTATION_ORDER_XYZ);
                }
                
                // Set scale
                ufbxw_vec3 scale = {1.0, 1.0, 1.0};
                if (get_map_vec3(env, head, "scale", &scale)) {
                    ufbxw_node_set_scaling(scene, node, scale);
                }
            }
            
            // Store node mesh_ids for later connection
            unsigned int *node_mesh_ids = (unsigned int*)enif_alloc(sizeof(unsigned int) * nodes_len);
            for (unsigned int i = 0; i < nodes_len; i++) {
                node_mesh_ids[i] = 0; // 0 = no mesh
            }
            
            // Set parent-child relationships and collect mesh_ids
            tail = nodes_list;
            for (unsigned int i = 0; i < nodes_len; i++) {
                if (!enif_get_list_cell(env, tail, &head, &tail)) break;
                
                // Get parent_id
                unsigned int parent_id;
                if (get_map_uint(env, head, "parent_id", &parent_id)) {
                    // Find parent node
                    for (unsigned int j = 0; j < nodes_len; j++) {
                        if (node_ids[j] == parent_id) {
                            ufbxw_node_set_parent(scene, node_handles[i], node_handles[j]);
                            break;
                        }
                    }
                }
                
                // Get mesh_id for later connection
                unsigned int mesh_id;
                if (get_map_uint(env, head, "mesh_id", &mesh_id)) {
                    node_mesh_ids[i] = mesh_id;
                }
            }
            
            // Parse meshes first (need mesh handles to connect to nodes)
            ERL_NIF_TERM meshes_list;
            ufbxw_mesh *mesh_handles = NULL;
            unsigned int *mesh_ids = NULL;
            unsigned int meshes_len = 0;
            
            if (get_map_list(env, scene_data_map, "meshes", &meshes_list)) {
                if (enif_get_list_length(env, meshes_list, &meshes_len)) {
                    mesh_handles = (ufbxw_mesh*)enif_alloc(sizeof(ufbxw_mesh) * meshes_len);
                    mesh_ids = (unsigned int*)enif_alloc(sizeof(unsigned int) * meshes_len);
                    
                    ERL_NIF_TERM head, tail = meshes_list;
                    for (unsigned int i = 0; i < meshes_len; i++) {
                if (!enif_get_list_cell(env, tail, &head, &tail)) break;
                
                // Create mesh
                ufbxw_mesh mesh = ufbxw_create_mesh(scene);
                mesh_handles[i] = mesh;
                
                // Get mesh ID
                unsigned int mesh_id;
                if (get_map_uint(env, head, "id", &mesh_id)) {
                    mesh_ids[i] = mesh_id;
                }
                
                // Set name
                char *name_str;
                size_t name_len;
                if (get_map_string(env, head, "name", &name_str, &name_len)) {
                    ufbxw_set_name_len(scene, mesh.id, name_str, name_len);
                }
                
                // Set vertices
                // Positions are flattened list [x1, y1, z1, x2, y2, z2, ...]
                ERL_NIF_TERM positions_list;
                if (get_map_list(env, head, "positions", &positions_list)) {
                    unsigned int positions_len;
                    if (enif_get_list_length(env, positions_list, &positions_len)) {
                        size_t vertex_count = positions_len / 3;
                        if (vertex_count > 0 && positions_len % 3 == 0) {
                            ufbxw_vec3 *vertices = (ufbxw_vec3*)enif_alloc(sizeof(ufbxw_vec3) * vertex_count);
                            
                            ERL_NIF_TERM pos_head, pos_tail = positions_list;
                            for (size_t v = 0; v < vertex_count; v++) {
                                double x = 0.0, y = 0.0, z = 0.0;
                                // Get x
                                if (enif_get_list_cell(env, pos_tail, &pos_head, &pos_tail)) {
                                    enif_get_double(env, pos_head, &x);
                                }
                                // Get y
                                if (enif_get_list_cell(env, pos_tail, &pos_head, &pos_tail)) {
                                    enif_get_double(env, pos_head, &y);
                                }
                                // Get z
                                if (enif_get_list_cell(env, pos_tail, &pos_head, &pos_tail)) {
                                    enif_get_double(env, pos_head, &z);
                                }
                                vertices[v].x = x;
                                vertices[v].y = y;
                                vertices[v].z = z;
                            }
                            
                            ufbxw_vec3_buffer vertices_buf = ufbxw_copy_vec3_array(scene, vertices, vertex_count);
                            ufbxw_mesh_set_vertices(scene, mesh, vertices_buf);
                            
                            enif_free(vertices);
                        }
                    }
                }
                
                // Set indices/triangles
                ERL_NIF_TERM indices_list;
                if (get_map_list(env, head, "indices", &indices_list)) {
                    unsigned int indices_len;
                    if (enif_get_list_length(env, indices_list, &indices_len)) {
                        int32_t *indices = (int32_t*)enif_alloc(sizeof(int32_t) * indices_len);
                        
                        ERL_NIF_TERM idx_head, idx_tail = indices_list;
                        for (unsigned int j = 0; j < indices_len; j++) {
                            if (!enif_get_list_cell(env, idx_tail, &idx_head, &idx_tail)) break;
                            unsigned int idx_val;
                            if (enif_get_uint(env, idx_head, &idx_val)) {
                                indices[j] = (int32_t)idx_val;
                            }
                        }
                        
                        ufbxw_int_buffer indices_buf = ufbxw_copy_int_array(scene, indices, indices_len);
                        ufbxw_mesh_set_triangles(scene, mesh, indices_buf);
                        
                        enif_free(indices);
                    }
                }
                
                // Set normals
                // Normals are flattened list [nx1, ny1, nz1, nx2, ny2, nz2, ...]
                ERL_NIF_TERM normals_list;
                if (get_map_list(env, head, "normals", &normals_list)) {
                    unsigned int normals_len;
                    if (enif_get_list_length(env, normals_list, &normals_len)) {
                        size_t normal_count = normals_len / 3;
                        if (normal_count > 0 && normals_len % 3 == 0) {
                            ufbxw_vec3 *normals = (ufbxw_vec3*)enif_alloc(sizeof(ufbxw_vec3) * normal_count);
                            
                            ERL_NIF_TERM norm_head, norm_tail = normals_list;
                            for (size_t n = 0; n < normal_count; n++) {
                                double x = 0.0, y = 0.0, z = 1.0;
                                // Get x
                                if (enif_get_list_cell(env, norm_tail, &norm_head, &norm_tail)) {
                                    enif_get_double(env, norm_head, &x);
                                }
                                // Get y
                                if (enif_get_list_cell(env, norm_tail, &norm_head, &norm_tail)) {
                                    enif_get_double(env, norm_head, &y);
                                }
                                // Get z
                                if (enif_get_list_cell(env, norm_tail, &norm_head, &norm_tail)) {
                                    enif_get_double(env, norm_head, &z);
                                }
                                normals[n].x = x;
                                normals[n].y = y;
                                normals[n].z = z;
                            }
                            
                            ufbxw_vec3_buffer normals_buf = ufbxw_copy_vec3_array(scene, normals, normal_count);
                            ufbxw_mesh_set_normals(scene, mesh, normals_buf, UFBXW_ATTRIBUTE_MAPPING_VERTEX);
                            
                            enif_free(normals);
                        }
                    }
                }
                
                // Set UVs
                // Texcoords are flattened list [u1, v1, u2, v2, ...]
                ERL_NIF_TERM texcoords_list;
                if (get_map_list(env, head, "texcoords", &texcoords_list)) {
                    unsigned int texcoords_len;
                    if (enif_get_list_length(env, texcoords_list, &texcoords_len)) {
                        size_t uv_count = texcoords_len / 2;
                        if (uv_count > 0 && texcoords_len % 2 == 0) {
                            ufbxw_vec2 *uvs = (ufbxw_vec2*)enif_alloc(sizeof(ufbxw_vec2) * uv_count);
                            
                            ERL_NIF_TERM uv_head, uv_tail = texcoords_list;
                            for (size_t u = 0; u < uv_count; u++) {
                                double u_val = 0.0, v_val = 0.0;
                                // Get u
                                if (enif_get_list_cell(env, uv_tail, &uv_head, &uv_tail)) {
                                    enif_get_double(env, uv_head, &u_val);
                                }
                                // Get v
                                if (enif_get_list_cell(env, uv_tail, &uv_head, &uv_tail)) {
                                    enif_get_double(env, uv_head, &v_val);
                                }
                                uvs[u].x = u_val;
                                uvs[u].y = v_val;
                            }
                            
                            ufbxw_vec2_buffer uvs_buf = ufbxw_copy_vec2_array(scene, uvs, uv_count);
                            ufbxw_mesh_set_uvs(scene, mesh, 0, uvs_buf, UFBXW_ATTRIBUTE_MAPPING_VERTEX);
                            
                            enif_free(uvs);
                        }
                    }
                }
                }
            }
            }
            
            // Connect meshes to nodes
            if (mesh_handles && node_handles) {
                for (unsigned int i = 0; i < nodes_len; i++) {
                    if (node_mesh_ids[i] != 0) {
                        // Find mesh with matching ID
                        for (unsigned int j = 0; j < meshes_len; j++) {
                            if (mesh_ids[j] == node_mesh_ids[i]) {
                                // Connect mesh to node
                                ufbxw_node_set_attribute(scene, node_handles[i], mesh_handles[j].id);
                                break;
                            }
                        }
                    }
                }
            }
            
            if (mesh_handles) enif_free(mesh_handles);
            if (mesh_ids) enif_free(mesh_ids);
            enif_free(node_mesh_ids);
            enif_free(node_handles);
            enif_free(node_ids);
        }
    } else {
        // No nodes, but still parse meshes
        ERL_NIF_TERM meshes_list;
        if (get_map_list(env, scene_data_map, "meshes", &meshes_list)) {
            unsigned int meshes_len;
            if (enif_get_list_length(env, meshes_list, &meshes_len)) {
                ufbxw_mesh *mesh_handles = (ufbxw_mesh*)enif_alloc(sizeof(ufbxw_mesh) * meshes_len);
                unsigned int *mesh_ids = (unsigned int*)enif_alloc(sizeof(unsigned int) * meshes_len);
                
                ERL_NIF_TERM head, tail = meshes_list;
                for (unsigned int i = 0; i < meshes_len; i++) {
                    if (!enif_get_list_cell(env, tail, &head, &tail)) break;
                    
                    // Create mesh
                    ufbxw_mesh mesh = ufbxw_create_mesh(scene);
                    mesh_handles[i] = mesh;
                    
                    // Get mesh ID
                    unsigned int mesh_id;
                    if (get_map_uint(env, head, "id", &mesh_id)) {
                        mesh_ids[i] = mesh_id;
                    }
                    
                    // Set name
                    char *name_str;
                    size_t name_len;
                    if (get_map_string(env, head, "name", &name_str, &name_len)) {
                        ufbxw_set_name_len(scene, mesh.id, name_str, name_len);
                    }
                    
                    // Set vertices (same code as above)
                    ERL_NIF_TERM positions_list;
                    if (get_map_list(env, head, "positions", &positions_list)) {
                        unsigned int positions_len;
                        if (enif_get_list_length(env, positions_list, &positions_len)) {
                            size_t vertex_count = positions_len / 3;
                            if (vertex_count > 0 && positions_len % 3 == 0) {
                                ufbxw_vec3 *vertices = (ufbxw_vec3*)enif_alloc(sizeof(ufbxw_vec3) * vertex_count);
                                
                                ERL_NIF_TERM pos_head, pos_tail = positions_list;
                                for (size_t v = 0; v < vertex_count; v++) {
                                    double x = 0.0, y = 0.0, z = 0.0;
                                    if (enif_get_list_cell(env, pos_tail, &pos_head, &pos_tail)) {
                                        enif_get_double(env, pos_head, &x);
                                    }
                                    if (enif_get_list_cell(env, pos_tail, &pos_head, &pos_tail)) {
                                        enif_get_double(env, pos_head, &y);
                                    }
                                    if (enif_get_list_cell(env, pos_tail, &pos_head, &pos_tail)) {
                                        enif_get_double(env, pos_head, &z);
                                    }
                                    vertices[v].x = x;
                                    vertices[v].y = y;
                                    vertices[v].z = z;
                                }
                                
                                ufbxw_vec3_buffer vertices_buf = ufbxw_copy_vec3_array(scene, vertices, vertex_count);
                                ufbxw_mesh_set_vertices(scene, mesh, vertices_buf);
                                
                                enif_free(vertices);
                            }
                        }
                    }
                    
                    // Set indices
                    ERL_NIF_TERM indices_list;
                    if (get_map_list(env, head, "indices", &indices_list)) {
                        unsigned int indices_len;
                        if (enif_get_list_length(env, indices_list, &indices_len)) {
                            int32_t *indices = (int32_t*)enif_alloc(sizeof(int32_t) * indices_len);
                            
                            ERL_NIF_TERM idx_head, idx_tail = indices_list;
                            for (unsigned int j = 0; j < indices_len; j++) {
                                if (!enif_get_list_cell(env, idx_tail, &idx_head, &idx_tail)) break;
                                unsigned int idx_val;
                                if (enif_get_uint(env, idx_head, &idx_val)) {
                                    indices[j] = (int32_t)idx_val;
                                }
                            }
                            
                            ufbxw_int_buffer indices_buf = ufbxw_copy_int_array(scene, indices, indices_len);
                            ufbxw_mesh_set_triangles(scene, mesh, indices_buf);
                            
                            enif_free(indices);
                        }
                    }
                }
                
                enif_free(mesh_handles);
                enif_free(mesh_ids);
            }
        }
    }
    
    // Parse materials (basic support)
    ERL_NIF_TERM materials_list;
    if (get_map_list(env, scene_data_map, "materials", &materials_list)) {
        unsigned int materials_len;
        if (enif_get_list_length(env, materials_list, &materials_len)) {
            ERL_NIF_TERM head, tail = materials_list;
            for (unsigned int i = 0; i < materials_len; i++) {
                if (!enif_get_list_cell(env, tail, &head, &tail)) break;
                
                // Create material element
                ufbxw_id material_id = ufbxw_create_element(scene, UFBXW_ELEMENT_MATERIAL);
                
                // Set name
                char *name_str;
                size_t name_len;
                if (get_map_string(env, head, "name", &name_str, &name_len)) {
                    ufbxw_set_name_len(scene, material_id, name_str, name_len);
                }
                
                // Set diffuse color
                ufbxw_vec3 diffuse = {1.0, 1.0, 1.0};
                if (get_map_vec3(env, head, "diffuse_color", &diffuse)) {
                    ufbxw_set_vec3(scene, material_id, "DiffuseColor", diffuse);
                }
            }
        }
    }
    
    return scene;
}

// Write FBX file NIF
static ERL_NIF_TERM write_fbx_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    ErlNifBinary file_path_bin;
    ERL_NIF_TERM scene_data_map;
    ERL_NIF_TERM format_atom;
    ufbxw_error error = {0};
    ufbxw_scene *scene;
    
    // Get arguments
    if (argc != 3) {
        return enif_make_badarg(env);
    }
    
    if (!enif_inspect_binary(env, argv[0], &file_path_bin)) {
        return enif_make_badarg(env);
    }
    
    if (!enif_is_map(env, argv[1])) {
        return enif_make_badarg(env);
    }
    
    scene_data_map = argv[1];
    
    // Get format (binary or ascii)
    format_atom = argv[2];
    
    // Null-terminate file path
    char file_path[file_path_bin.size + 1];
    memcpy(file_path, file_path_bin.data, file_path_bin.size);
    file_path[file_path_bin.size] = '\0';
    
    // Build ufbxw_scene from Elixir map
    scene = build_ufbxw_scene_from_map(env, scene_data_map);
    if (!scene) {
        return enif_make_tuple2(env,
            enif_make_atom(env, "error"),
            enif_make_string(env, "Failed to create ufbxw_scene", ERL_NIF_LATIN1));
    }
    
    // Determine format
    ufbxw_save_format format = UFBXW_SAVE_FORMAT_BINARY;
    char atom_buf[256];
    if (enif_get_atom(env, format_atom, atom_buf, sizeof(atom_buf), ERL_NIF_LATIN1)) {
        if (strcmp(atom_buf, "ascii") == 0) {
            format = UFBXW_SAVE_FORMAT_ASCII;
        }
    }
    
    // Save options
    ufbxw_save_opts opts = {0};
    opts.format = format;
    opts.version = 7400; // FBX 7.4
    
    // Save file
    bool success = ufbxw_save_file(scene, file_path, &opts, &error);
    
    // Free scene
    ufbxw_free_scene(scene);
    
    if (!success) {
        char error_msg[256];
        // ufbxw_error.description is a char array, not a struct with .data
        // Ensure null termination and handle truncation safely
        int written = snprintf(error_msg, sizeof(error_msg), "Failed to save FBX: %s", 
                               error.description);
        if (written >= (int)sizeof(error_msg)) {
            error_msg[sizeof(error_msg) - 1] = '\0';
        }
        return enif_make_tuple2(env,
            enif_make_atom(env, "error"),
            enif_make_string(env, error_msg, ERL_NIF_LATIN1));
    }
    
    // Return success
    return enif_make_tuple2(env,
        enif_make_atom(env, "ok"),
        enif_make_string(env, file_path, ERL_NIF_LATIN1));
}

static ErlNifFunc nif_funcs[] = {
    {"load_fbx", 1, load_fbx_nif, 0},
    {"load_fbx_binary", 1, load_fbx_binary_nif, 0},
    {"write_fbx", 3, write_fbx_nif, 0}
};

ERL_NIF_INIT(Elixir.AriaFbx.Nif, nif_funcs, NULL, NULL, NULL, NULL)
