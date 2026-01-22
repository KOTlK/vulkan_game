#pragma once

#include "basic.h"
#include "assert.h"
#include "malloc.h"
#include <string.h>
#define BITMAP_IMPLEMENTATION
#include "components.h"

import list;
import hash_table;
import bitmap;

#define COMPONENTS_ADD_REALLOC_COUNT 256

#define COMPONENTS_MALLOC(type, size) (type*)malloc(size)
#define COMPONENTS_FREE(ptr) free(ptr)
#define COMPONENTS_REALLOC(ptr, size) realloc(ptr, size)

#define COMPONENTS_INITIAL_SPARSE_LENGTH 1024
#define COMPONENTS_INITIAL_DENSE_LENGTH  128

typedef u32 Entity;
static Archetype Archetype_Zero = {};

struct EntityHandle {
    u32 id;
    u32 generation;
};

struct EntitySlot {
    u32       generation;
    Archetype archetype;
};

struct EntityManager {
    HashTable<Archetype, List<Entity>> archetypes;
    EntitySlot* entities;
    u32*        free;
    u32         entities_count;
    u32         entities_length;
    u32         free_count;
};

struct ComponentTable {
    void* dense;
    u32*  sparse;
    u32*  entity_by_component_id;
    u32   dense_count;
    u32   dense_length;
    u32   sparse_length;
    u32   component_size;
};


void         entity_manager_make(EntityManager* em);

EntityHandle entity_create(EntityManager* em);
bool         entity_is_alive(EntityManager* em, EntityHandle handle);
void         entity_destroy(EntityManager* em, EntityHandle handle);
Archetype&   entity_get_archetype(EntityManager* em, Entity entity);

void         archetype_remove(EntityManager* em, Entity entity);
void         archetype_add(EntityManager* em, Entity entity);

static inline ComponentTable component_table_make(u32 component_size);
static inline void           component_table_free(ComponentTable* table);
static inline void           component_table_realloc_sparse(ComponentTable* table, u32 size);
static inline void           component_table_realloc_dense(ComponentTable* table, u32 size);
static inline bool           component_table_has(ComponentTable* table, Entity entity);
static inline void           component_table_remove(ComponentTable* table, Entity entity);

template <typename T>
static inline T*   component_table_add(ComponentTable* table, Entity entity, T component);
template <typename T>
static inline T*   component_table_get(ComponentTable* table, Entity entity);
template <typename T>
static inline void component_table_set(ComponentTable* table, Entity handle, T component);


static inline ComponentTable component_table_make(u32 component_size) {
    ComponentTable table = {
        .dense                  = COMPONENTS_MALLOC(void, component_size * COMPONENTS_INITIAL_DENSE_LENGTH),
        .sparse                 = COMPONENTS_MALLOC(u32, sizeof(u32) * COMPONENTS_INITIAL_SPARSE_LENGTH),
        .entity_by_component_id = COMPONENTS_MALLOC(u32, sizeof(u32) * COMPONENTS_INITIAL_DENSE_LENGTH),
        .dense_count            = 1,
        .dense_length           = COMPONENTS_INITIAL_DENSE_LENGTH,
        .sparse_length          = COMPONENTS_INITIAL_SPARSE_LENGTH,
        .component_size         = component_size,
    };

    memset(table.sparse, 0, sizeof(u32) * COMPONENTS_INITIAL_SPARSE_LENGTH);
    memset(table.entity_by_component_id, 0, sizeof(u32) * COMPONENTS_INITIAL_DENSE_LENGTH);

    return table;
}

static inline void component_table_free(ComponentTable* table) {
    Assert(table, "Cannot free NULL component table.");

    COMPONENTS_FREE(table->dense);
    COMPONENTS_FREE(table->sparse);
    COMPONENTS_FREE(table->entity_by_component_id);
}

static inline void component_table_realloc_sparse(ComponentTable* table, u32 size) {
    Assert(table, "Cannot realloc NULL component table.");

    table->sparse = (u32*)COMPONENTS_REALLOC(table->sparse, sizeof(u32) * size);

    Assert(table->sparse, "Cannot reallocate sparse data for component table");

    table->sparse_length = size;
}

static inline void component_table_realloc_dense(ComponentTable* table, u32 size) {
    Assert(table, "Cannot realloc NULL component table.");
    table->dense                  = COMPONENTS_REALLOC(table->dense, table->component_size * size);
    table->entity_by_component_id = (u32*)COMPONENTS_REALLOC(table->entity_by_component_id, sizeof(u32) * size);

    Assert(table->dense, "Cannot reallocate dense data for component table");
    Assert(table->entity_by_component_id, "Cannot reallocate entity data for component table");

    table->dense_length = size;
}

template <typename T>
static inline T* component_table_add(ComponentTable* table, Entity entity, T component) {
    Assert(entity != 0, "Cannot use zero entity.");
    u32 id        = table->dense_count;

    if (id >= table->dense_length) {
        component_table_realloc_dense(table, id + COMPONENTS_ADD_REALLOC_COUNT);
    }

    T* dense = (T*)table->dense;

    dense[id] = component;

    if (entity >= table->sparse_length) {
        component_table_realloc_sparse(table, entity + COMPONENTS_ADD_REALLOC_COUNT);
    }

    table->sparse[entity] = id;

    table->entity_by_component_id[id] = entity;

    table->dense_count++;

    return &dense[id];
}

template <typename T>
static inline T* component_table_get(ComponentTable* table, Entity entity) {
    Assertf(component_table_has(table, entity), "Entity (%d) you want to get does not have the component.", entity);
    T* dense = (T*)table->dense;
    return &dense[table->sparse[entity]];
}

static inline bool component_table_has(ComponentTable* table, Entity entity) {
    Assert(entity != 0, "Cannot use zero entity.");
    if (table->sparse_length <= entity) return false;

    return table->sparse[entity] != 0;
}

template <typename T>
static inline void component_table_set(ComponentTable* table, Entity entity, T component) {
    Assert(entity != 0, "Cannot use zero entity.");
    Assertf(component_table_has(table, entity), "Cannot set component. Entity does not have the component attached.");
    T* dense = (T*)table->dense;
    dense[table->sparse[entity]] = component;
}

static inline void component_table_remove(ComponentTable* table, Entity entity) {
    Assertf(component_table_has(table, entity), "Entity (%d) you want to remove does not have the component.", entity);
    u32 index       = table->sparse[entity];
    u32 last        = table->dense_count - 1;
    u32 last_entity = table->entity_by_component_id[last];

    memcpy((char*)table->dense + (index * table->component_size), 
           (char*)table->dense + (last * table->component_size), 
           table->component_size);

    memset((char*)table->dense + last * table->component_size, 
            0, 
            table->component_size);

    table->entity_by_component_id[index] = last_entity;
    table->entity_by_component_id[last]  = 0;
    table->sparse[entity]                = 0;
    table->sparse[last_entity]           = index;

    table->dense_count--;
}

static inline void entity_print_components(EntityManager* em, Entity entity) {
    Assert(entity != 0, "Cannot use zero entity.");
    auto archetype = em->entities[entity].archetype;

    printf("Entity: %d. Components: ", entity);

    for (u32 i = 0; i < COMPONENTS_COUNT; i++) {
        if (bitmap_test_bit(archetype, i)) {
            printf("%s, ", Component_Name_By_Bit[i]);
        }
    }
    printf("\n");
}