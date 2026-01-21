#pragma once

#include "allocator.h"

struct Context {
	Allocator* allocator;
};

#define $ctx (&Global_Context)

extern Context Global_Context;