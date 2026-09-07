#ifndef MAIN_MEM_H
#define MAIN_MEM_H

#include "types.h"

// game/main_mem.cpp heap. Callers pass their source location.
void* mem_alloc(u32 size, const char* file, int line, int a, int b);

#define MEM_ALLOC(size, a, b) mem_alloc(size, __FILE__, __LINE__, a, b)

#endif
