#include "context.h"
#include "basic.h"

#define ADDITIONAL_CONTEXT_COUNT 64

static Context Global_Context = {
	.allocator = Allocator_Persistent,
};