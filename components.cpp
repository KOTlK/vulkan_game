#include "component_system.h"
#include "components.h"

ComponentTable TestComponent_s = component_table_make(sizeof(TestComponent));
u32 TestComponent_bit = 0;

ComponentTable Transform_s = component_table_make(sizeof(Transform));
u32 Transform_bit = 1;

ComponentTable* All_Components[] = {
  &TestComponent_s,
  &Transform_s,
};

static inline ComponentTable* get_component_table_by_bit(u32 bit) {
  return All_Components[bit];
}
