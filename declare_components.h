#pragma once

#include "types.h"
#include "component_system.h"
#include "render.h"

struct TestComponent {
    u32 a;
    u16 b;
};

#define BEGIN_ITERATE_COMPONENT(type) \
    u32   components_count       = type##_s.dense_count; \
    type* dense                  = (type*)type##_s.dense;\
    for (u32 component_index = 1; component_index < components_count; component_index++) {\
        type* component = &dense[component_index];\

#define END_ITERATE_COMPONENT() }

#define BEGIN_ITERATE_COMPONENT_WITH_ENTITY(type) \
    u32   components_count       = type##_s.dense_count; \
    type* dense                  = (type*)type##_s.dense;\
    u32*  entity_by_component_id = type##_s.entity_by_component_id;\
    for (u32 component_index = 1; component_index < components_count; component_index++) {\
        u32   entity    = entity_by_component_id[component_index];\
        type* component = &dense[component_index];\

#define END_ITERATE_COMPONENT_WITH_ENTITY() }

#define ADD_COMPONENT(type, em, entity, component) \
    component_table_add(&type##_s, entity, component);\
    ENTITY_SET_COMPONENT_BIT(type, em, entity);\

#define REMOVE_COMPONENT(type, em, entity) \
    component_table_remove(&type##_s, entity);\
    ENTITY_CLEAR_COMPONENT_BIT(type, em, entity);\

#define HAS_COMPONENT(type, entity)    component_table_has(&type##_s, entity)
#define GET_COMPONENT(type, entity)    component_table_get(&type##_s, entity)

#define GET_COMPONENT_BIT(type)    type##_bit
#define COMPONENTS_GET_COUNT(type) type##_s.dense_count

#define ENTITY_SET_COMPONENT_BIT(type, em, handle) \
    Assertf(entity_is_alive(em, handle), "Cannot set bit for dead entity. Id: %d.", handle.id);\
    bitmap_set_bit(em->entities[handle.id].archetype, GET_COMPONENT_BIT(type));\

#define ENTITY_TEST_COMPONENT_BIT(type, em, handle) bitmap_test_bit(em->entities[handle.id].archetype, GET_COMPONENT_BIT(type))

#define ENTITY_CLEAR_COMPONENT_BIT(type, em, handle) \
    Assertf(entity_is_alive(em, handle), "Cannot clear bit for dead entity. Id: %d.", handle.id);\
    Assertf(ENTITY_TEST_COMPONENT_BIT(type, em, handle), "Entity does not have the component, you want to remove. Component bit: %d", GET_COMPONENT_BIT(type));\
    bitmap_clear_bit(em->entities[handle.id].archetype, GET_COMPONENT_BIT(type));\

extern ComponentTable* All_Components[];
extern ComponentTable* get_component_table_by_bit(u32 bit);

#DECLARE_COMPONENT(TestComponent)
#DECLARE_COMPONENT(Transform)