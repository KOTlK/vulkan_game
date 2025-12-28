#pragma once

#include "basic.h"

typedef u32 Entity;

struct EntityHandle {
    u32 id;
    u32 generation;
};

struct EntitySlot {
    u32 generation;
};

struct EntityManager {
    EntitySlot* entities;
    u32*        free;
    u32         entities_count;
    u32         entities_length;
    u32         free_count;
};

void   entity_manager_make(EntityManager* em);

EntityHandle entity_create(EntityManager* em);
bool         entity_is_alive(EntityManager* em, EntityHandle handle);
void         entity_destroy(EntityManager* em, EntityHandle handle);