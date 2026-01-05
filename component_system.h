#pragma once

#include "basic.h"
#include "assert.h"
#include "memory.h"
#include "entities.h"

#define COMPONENTS_ADD_REALLOC_COUNT 256

#define COMPONENTS_MALLOC(type, size) (type*)malloc(size)
#define COMPONENTS_FREE(ptr) free(ptr)
#define COMPONENTS_REALLOC(ptr, size) realloc(ptr, size)

#define COMPONENTS_INITIAL_SPARSE_LENGTH 1024
#define COMPONENTS_INITIAL_DENSE_LENGTH  128

struct ComponentTable {
    void*      dense;
    u32*       sparse;
    u32*       entity_by_component_id;
    u32        dense_count;
    u32        dense_length;
    u32        sparse_length;
    u32        component_size;
};

static inline ComponentTable component_table_make(u32 component_size);

static inline void component_table_free(ComponentTable* table);

static inline void component_table_realloc_sparse(ComponentTable* table, u32 size);

static inline void component_table_realloc_dense(ComponentTable* table, u32 size);

template <typename T>
static inline T* component_table_add(ComponentTable* table, EntityHandle entity, T component);

template <typename T>
static inline T* component_table_get(ComponentTable* table, EntityHandle entity);

template <typename T>
static inline void component_table_set(ComponentTable* table, EntityHandle handle, T component);

static inline bool component_table_has(ComponentTable* table, EntityHandle entity);

static inline void component_table_remove(ComponentTable* table, EntityHandle entity);

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
    // COMPONENTS_FREE(table->free);
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
static inline T* component_table_add(ComponentTable* table, EntityHandle handle, T component) {
    u32 id        = table->dense_count;
    u32 entity    = handle.id;

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
static inline T* component_table_get(ComponentTable* table, EntityHandle handle) {
    Assertf(component_table_has(table, handle), "Entity (%d) you want to get does not have the component.", handle.id);
    T* dense = (T*)table->dense;
    return dense[table->sparse[handle.id]];
}

static inline bool component_table_has(ComponentTable* table, EntityHandle handle) {
    if (table->sparse_length <= handle.id) return false;

    return table->sparse[handle.id] != 0;
}

template <typename T>
static inline void component_table_set(ComponentTable* table, EntityHandle handle, T component) {
    Assertf(component_table_has(table, handle), "Cannot set component. Entity does not have the component attached.");
    T* dense = (T*)table->dense;
    dense[table->sparse[handle.id]] = component;
}

static inline void component_table_remove(ComponentTable* table, EntityHandle handle) {
    u32 entity = handle.id;
    Assertf(component_table_has(table, handle), "Entity (%d) you want to remove does not have the component.", entity);
    u32 index       = table->sparse[entity];
    u32 last        = table->dense_count - 1;
    u32 last_entity = table->entity_by_component_id[last];

    memcpy((char*)table->dense + (index * table->component_size), (char*)table->dense + (last * table->component_size), table->component_size);
    memset((char*)table->dense + last * table->component_size, 0, table->component_size);
    // table->dense[index * table->component_size] = table->dense[last * component_size];
    // table->dense[last * table->component_size]  = {};
    table->entity_by_component_id[index] = last_entity;
    table->entity_by_component_id[last]  = 0;
    table->sparse[entity]                = 0;
    table->sparse[last_entity]           = index;

    table->dense_count--;
}