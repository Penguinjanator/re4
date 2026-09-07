#ifndef EPRINTF_H
#define EPRINTF_H

#include "types.h"

// Debug text output (game/eprintf.cpp).
void eprintf(int x, int y, int color, int a, const char* fmt, ...);
void eprintf2(int x, int y, int a, int b, int c, int d, const char* fmt, ...);

#endif
