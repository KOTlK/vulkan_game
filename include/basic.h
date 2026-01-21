#pragma once

#include "allocator.h"

#define null NULL

extern Allocator* Allocator_Persistent;
extern Allocator* Allocator_Temp      ;

static inline
void
free_temp_allocator() {
    Allocator_Temp->clear();
}