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

#define BEGIN_COMPONENTS_DECLARATION() \
    struct Components {\

#define END_COMPONENTS_DECLARATION() \
    };\
    static Components All_Components;\

#define DECLARE_COMPONENT(type) ComponentTable<type> type##_s = component_table_make<type>();

#define BEGIN_ITERATE_COMPONENT(type) \
    u32   components_count       = All_Components.type##_s.dense_count; \
    type* dense                  = All_Components.type##_s.dense;\
    for (u32 component_index = 1; component_index < components_count; component_index++) {\
        type* component = &dense[component_index];\

#define END_ITERATE_COMPONENT() }

#define BEGIN_ITERATE_COMPONENT_WITH_ENTITY(type) \
    u32   components_count       = All_Components.type##_s.dense_count; \
    type* dense                  = All_Components.type##_s.dense;\
    u32*  entity_by_component_id = All_Components.type##_s.entity_by_component_id;\
    for (u32 component_index = 1; component_index < components_count; component_index++) {\
        u32   entity    = entity_by_component_id[component_index];\
        type* component = &dense[component_index];\

#define END_ITERATE_COMPONENT_WITH_ENTITY() }

#define ADD_COMPONENT(type, entity)    component_table_add(&All_Components.type##_s,    entity)
#define REMOVE_COMPONENT(type, entity) component_table_remove(&All_Components.type##_s, entity)
#define HAS_COMPONENT(type, entity)    component_table_has(&All_Components.type##_s,    entity)
#define GET_COMPONENT(type, entity)    component_table_get(&All_Components.type##_s,    entity)

#define COMPONENTS_GET_COUNT(type) All_Components.type##_s.dense_count

template <typename T>
struct ComponentTable {
    T*         dense;
    u32*       sparse;
    // u32*    free;
    u32*       entity_by_component_id;
    u32        dense_count;
    u32        dense_length;
    // u32     sparse_count;
    u32        sparse_length;
    // u32     free_count;
};

template <typename T>
static inline ComponentTable<T> component_table_make();

template <typename T>
static inline void component_table_free(ComponentTable<T>* table);

template <typename T>
static inline void component_table_realloc_sparse(ComponentTable<T>* table, u32 size);

template <typename T>
static inline void component_table_realloc_dense(ComponentTable<T>* table, u32 size);

template <typename T>
T* component_table_add(ComponentTable<T>* table, EntityHandle entity);

template <typename T>
T* component_table_get(ComponentTable<T>* table, EntityHandle entity);

template <typename T>
bool component_table_has(ComponentTable<T>* table, EntityHandle entity);

template <typename T>
void component_table_remove(ComponentTable<T>* table, EntityHandle entity);

template <typename T>
static inline ComponentTable<T> component_table_make() {
    ComponentTable<T> table = {
        .dense                  = COMPONENTS_MALLOC(T, sizeof(T) * COMPONENTS_INITIAL_DENSE_LENGTH),
        .sparse                 = COMPONENTS_MALLOC(u32, sizeof(u32) * COMPONENTS_INITIAL_SPARSE_LENGTH),
        .entity_by_component_id = COMPONENTS_MALLOC(u32, sizeof(u32) * COMPONENTS_INITIAL_DENSE_LENGTH),
        .dense_count            = 1,
        .dense_length           = COMPONENTS_INITIAL_DENSE_LENGTH,
        .sparse_length          = COMPONENTS_INITIAL_SPARSE_LENGTH,
    };

    memset(table.sparse, 0, sizeof(u32) * COMPONENTS_INITIAL_SPARSE_LENGTH);
    memset(table.entity_by_component_id, 0, sizeof(u32) * COMPONENTS_INITIAL_DENSE_LENGTH);

    return table;
}

template <typename T>
static inline void component_table_free(ComponentTable<T>* table) {
    Assert(table, "Cannot free NULL component table.");

    COMPONENTS_FREE(table->dense);
    COMPONENTS_FREE(table->sparse);
    COMPONENTS_FREE(table->free);
    COMPONENTS_FREE(table->entity_by_component_id);
}

template <typename T>
static inline void component_table_realloc_sparse(ComponentTable<T>* table, u32 size) {
    Assert(table, "Cannot realloc NULL component table.");

    table->sparse = (u32*)COMPONENTS_REALLOC(table->sparse, sizeof(u32) * size);

    Assert(table->sparse, "Cannot reallocate sparse data for component table");

    table->sparse_length = size;
}

template <typename T>
static inline void component_table_realloc_dense(ComponentTable<T>* table, u32 size) {
    Assert(table, "Cannot realloc NULL component table.");
    table->dense                  = (T*)COMPONENTS_REALLOC(table->dense, sizeof(T) * size);
    table->entity_by_component_id = (u32*)COMPONENTS_REALLOC(table->entity_by_component_id, sizeof(u32) * size);

    Assert(table->dense, "Cannot reallocate dense data for component table");
    Assert(table->entity_by_component_id, "Cannot reallocate entity data for component table");

    table->dense_length = size;
}

template <typename T>
T* component_table_add(ComponentTable<T>* table, EntityHandle handle) {
    T   component = {};
    u32 id        = table->dense_count;
    u32 entity    = handle.id;

    if (id >= table->dense_length) {
        component_table_realloc_dense(table, id + COMPONENTS_ADD_REALLOC_COUNT);
    }

    table->dense[id] = component;

    if (entity >= table->sparse_length) {
        component_table_realloc_sparse(table, entity + COMPONENTS_ADD_REALLOC_COUNT);
    }

    table->sparse[entity] = id;

    table->entity_by_component_id[id] = entity;

    table->dense_count++;

    return &table->dense[id];
}

template <typename T>
T* component_table_get(ComponentTable<T>* table, EntityHandle handle) {
    Assertf(component_table_has(table, handle), "Entity (%d) you want to get does not have the component.", handle.id);
    return &table->dense[table->sparse[handle.id]];
}

template <typename T>
bool component_table_has(ComponentTable<T>* table, EntityHandle handle) {
    if (table->sparse_length <= handle.id) return false;

    return table->sparse[handle.id] != 0;
}

template <typename T>
void component_table_remove(ComponentTable<T>* table, EntityHandle handle) {
    u32 entity = handle.id;
    Assertf(component_table_has(table, handle), "Entity (%d) you want to remove does not have the component.", entity);
    u32 index       = table->sparse[entity];
    u32 last        = table->dense_count - 1;
    u32 last_entity = table->entity_by_component_id[last];

    table->dense[index]                  = table->dense[last];
    table->dense[last]                   = {};
    table->entity_by_component_id[index] = last_entity;
    table->entity_by_component_id[last]  = 0;
    table->sparse[entity]                = 0;
    table->sparse[last_entity]           = index;

    table->dense_count--;
}