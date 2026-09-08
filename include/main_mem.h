#ifndef MAIN_MEM_H
#define MAIN_MEM_H

#include "types.h"

// game/main_mem.cpp heap. Callers pass their source location.
void* mem_alloc(u32 size, const char* file, int line, int a, int b);

void* mem_calloc(u32 size, const char* file, int line, int a, int b);

#define MEM_ALLOC(size, a, b) mem_alloc(size, __FILE__, __LINE__, a, b)
#define MEM_CALLOC(size, a, b) mem_calloc(size, __FILE__, __LINE__, a, b)

// Debug heap (CurrentDbgHeap). Debug_free is the out-of-line copy owned by main_mem.
void* Debug_alloc(u32 size, int flag);
void Debug_free(void* p);

extern "C" {
// game/memset_2.s
void memclr_asm(void* dst, u32 n);
void memset_asm(void* dst, int c, u32 n);
}

#endif
