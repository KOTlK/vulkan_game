#pragma once

#include "types.h"
#include "component_system.h"
#include "render.h"

struct TestComponent {
    u32 a;
    u16 b;
};

BEGIN_COMPONENTS_DECLARATION()
DECLARE_COMPONENT(TestComponent)
DECLARE_COMPONENT(Transform)
END_COMPONENTS_DECLARATION()