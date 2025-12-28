#pragma once

#include "types.h"
#include "component_system.h"

struct TestComponent {
    u32 a;
    u16 b;
};

BEGIN_COMPONENTS_DECLARATION()
DECLARE_COMPONENT(TestComponent)
END_COMPONENTS_DECLARATION()