#include "basic.h"
#include "arena.h"
#include "std_allocator.h"

static Allocator* Allocator_Persistent = new AllocatorPersistent();
static Allocator* Allocator_Temp       = new Arena();