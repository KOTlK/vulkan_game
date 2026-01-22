#include "basic.h"
#include "arena.h"
#include "std_allocator.h"

Allocator* Allocator_Persistent = new AllocatorPersistent();
Allocator* Allocator_Temp       = new Arena();