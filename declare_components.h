#pragma once

#include "bitmap.h"
#include "types.h"
#include "render.h"

struct ComponentTable;

struct TestComponent {
    u32 a;
    u16 b;
};

struct TestComponent2 {
    u32 c;
};

struct TestComponent3 {
    u32 a;
};

struct TestComponent4 {
    u32 a;
};

#define BEGIN_ITERATE_COMPONENT(em, type) \
    Archetype __query_mask = bitmap_make<ARCHETYPE_BIT_COUNT>(GET_COMPONENT_BIT(type));\
    \
    for (auto [__arch, __list] : em->archetypes) {\
        Archetype __intersection = bitmap_and(__arch, __query_mask);\
        if (__intersection != __query_mask) continue;\
        if (__list.count == 0) continue;\
        \
        for (auto entity : __list) {\
            type* type##_c = GET_COMPONENT(type, entity);\

#define END_ITERATE_COMPONENT() }}

#define BEGIN_ITERATE_COMPONENTS_2(em, type1, type2) \
    Archetype __query_mask = bitmap_make<ARCHETYPE_BIT_COUNT>(GET_COMPONENT_BIT(type1), GET_COMPONENT_BIT(type2));\
    \
    for (auto [__arch, __list] : em->archetypes) {\
        Archetype __intersection = bitmap_and(__arch, __query_mask);\
        if (__intersection != __query_mask) continue;\
        if (__list.count == 0) continue;\
        \
        for (auto entity : __list) {\
            type1* type1##_c = GET_COMPONENT(type1, entity);\
            type2* type2##_c = GET_COMPONENT(type2, entity);\

#define END_ITERATE_COMPONENTS_2() } }\

#define ADD_COMPONENT(type, em, entity, component) \
    archetype_remove(em, entity);\
    component_table_add(&type##_s, entity, component);\
    ENTITY_SET_COMPONENT_BIT(type, em, entity);\
    archetype_add(em, entity);\

#define REMOVE_COMPONENT(type, em, entity) \
    archetype_remove(em, entity);\
    component_table_remove(&type##_s, entity);\
    ENTITY_CLEAR_COMPONENT_BIT(type, em, entity);\
    archetype_add(em, entity);\

#define REMOVE_COMPONENT_IF_EXIST(type, em, entity) \
    if (HAS_COMPONENT(type, em, entity)) {\
        REMOVE_COMPONENT(type, em, entity);\
    }\

#define HAS_COMPONENT(type, em, entity) ENTITY_TEST_COMPONENT_BIT(type, em, entity)
#define GET_COMPONENT(type, entity)     component_table_get<type>(&type##_s, entity)

#define GET_COMPONENT_BIT(type)    type##_bit
#define COMPONENTS_GET_COUNT(type) type##_s.dense_count

#define ENTITY_SET_COMPONENT_BIT(type, em, entity) \
    bitmap_set_bit(em->entities[entity].archetype, GET_COMPONENT_BIT(type));\

#define ENTITY_TEST_COMPONENT_BIT(type, em, entity) bitmap_test_bit(em->entities[entity].archetype, GET_COMPONENT_BIT(type))

#define ENTITY_CLEAR_COMPONENT_BIT(type, em, entity) \
    Assertf(ENTITY_TEST_COMPONENT_BIT(type, em, entity), "Entity(%d) does not have the component, you want to remove. Component bit: %d, name: " #type "", entity, GET_COMPONENT_BIT(type));\
    bitmap_clear_bit(em->entities[entity].archetype, GET_COMPONENT_BIT(type));\


extern ComponentTable* All_Components[];
extern const char*     Component_Name_By_Bit[];
extern ComponentTable* get_component_table_by_bit(u32 bit);

#DECLARE_COMPONENT(TestComponent)
#DECLARE_COMPONENT(Transform)
#DECLARE_COMPONENT(TestComponent2)
#DECLARE_COMPONENT(Renderer2D)
#DECLARE_COMPONENT(TestComponent3)
#DECLARE_COMPONENT(TestComponent4)
