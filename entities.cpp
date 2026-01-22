#define BITMAP_IMPLEMENTATION

#include "component_system.h"
#include "assert.h"
#include "memory.h"
#include "component_system.h"
#include "components.h"

import math;
import bitmap;

#define START_ENTITY_LENGTH 1024
#define REALLOC_STEP 256

void entity_manager_make(EntityManager* em) {
    Assert(em, "Entity manager is null");

    em->archetypes      = table_make<Archetype, List<Entity>>();
    em->entities        = (EntitySlot*)malloc(sizeof(EntitySlot) * START_ENTITY_LENGTH);
    em->free            = (u32*)malloc(sizeof(u32) * START_ENTITY_LENGTH);
    em->entities_count  = 1;
    em->entities_length = START_ENTITY_LENGTH;
    em->free_count      = 0;

    Assert(em->entities, "Cannot allocate memory for entities array");
    Assert(em->free, "Cannot allocate memory for free entities array");

    memset(em->entities, 0, sizeof(EntitySlot) * START_ENTITY_LENGTH);
    memset(em->free, 0, sizeof(u32) * START_ENTITY_LENGTH);
}

EntityHandle entity_create(EntityManager* em) {
    u32 id = 0;

    if (em->free_count > 0) {
        id = em->free[--em->free_count];
    } else {
        id = em->entities_count++;

        if (id >= em->entities_length) {
            u32 new_len = em->entities_length + REALLOC_STEP;
            em->entities = (EntitySlot*)realloc(em->entities, sizeof(EntitySlot) * new_len);
            em->free = (u32*)realloc(em->free, sizeof(u32) * new_len);

            em->entities_length = new_len;

            Assert(em->entities, "Cannot reallocate memory for entities");
            Assert(em->free, "Cannot reallocate memory for entities");
        }
    }
    
    EntityHandle handle = {
        .id = id,
        .generation = em->entities[id].generation
    };

    return handle;
}

bool entity_is_alive(EntityManager* em, EntityHandle handle) {
    Assertf(handle.id < em->entities_length, "Entity index is outside the bounds of the manager capacity. Id: %d, capacity: %d", handle.id, em->entities_length);
    return em->entities[handle.id].generation == handle.generation;
}

void entity_destroy(EntityManager* em, EntityHandle handle) {
    Assertf(handle.id < em->entities_length, "Cannot destroy an entity. Entity index is outside the bounds of the manager capacity. Id: %d, capacity: %d", handle.id, em->entities_length);

    Assert(entity_is_alive(em, handle), "Cannot destroy dead entity");

    em->entities[handle.id].generation++;

    archetype_remove(em, handle.id);

    for (u32 i = 0; i < COMPONENTS_COUNT; i++) {
        if (bitmap_test_bit(em->entities[handle.id].archetype, i)) {
            auto table = get_component_table_by_bit(i);
            archetype_remove(em, handle.id);
            component_table_remove(table, handle.id);
            bitmap_clear_bit(em->entities[handle.id].archetype, i);
        }
    }

    if (em->free_count > 0 && 
        em->free[em->free_count - 1] < handle.id) {
        em->free[em->free_count] = em->free[em->free_count - 1];
        em->free[em->free_count - 1] = handle.id;
    } else {
        em->free[em->free_count] = handle.id;
    }

    em->free_count++;
}

Archetype& entity_get_archetype(EntityManager* em, Entity entity) {
    return em->entities[entity].archetype;
}

void archetype_remove(EntityManager* em, Entity entity) {
    auto archetype = entity_get_archetype(em, entity);
    if (archetype == Archetype_Zero) return;

    if (table_contains(&em->archetypes, archetype)) {
        auto list = table_get_ptr(&em->archetypes, archetype);
        list_remove(list, entity);
        if (list->count == 0) {
            list_free(list);
            table_remove(&em->archetypes, archetype);
        }
    }
}

void archetype_add(EntityManager* em, Entity entity) {
    auto archetype = entity_get_archetype(em, entity);
    
    if (table_contains(&em->archetypes, archetype) == false) {
        auto list = list_make<Entity>();
        table_add(&em->archetypes, archetype, list);
    }

    list_append(table_get_ptr(&em->archetypes, archetype), entity);
}