#include "component_system.h"
#include "components.h"

ComponentTable TestComponent_s = component_table_make(sizeof(TestComponent));
u32 TestComponent_bit = 0;

ComponentTable Transform_s = component_table_make(sizeof(Transform));
u32 Transform_bit = 1;

ComponentTable TestComponent2_s = component_table_make(sizeof(TestComponent2));
u32 TestComponent2_bit = 2;

ComponentTable Renderer2D_s = component_table_make(sizeof(Renderer2D));
u32 Renderer2D_bit = 3;

ComponentTable TestComponent3_s = component_table_make(sizeof(TestComponent3));
u32 TestComponent3_bit = 4;

ComponentTable TestComponent4_s = component_table_make(sizeof(TestComponent4));
u32 TestComponent4_bit = 5;

ComponentTable* All_Components[] = {
  &TestComponent_s,
  &Transform_s,
  &TestComponent2_s,
  &Renderer2D_s,
  &TestComponent3_s,
  &TestComponent4_s,
};
const char* Component_Name_By_Bit[] = {
  "TestComponent",
  "Transform",
  "TestComponent2",
  "Renderer2D",
  "TestComponent3",
  "TestComponent4",
};

ComponentTable* get_component_table_by_bit(u32 bit) {
  return All_Components[bit];
}
