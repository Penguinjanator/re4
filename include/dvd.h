#ifndef DVD_H
#define DVD_H

#include "types.h"

// DVD read queue (game/dvd.cpp, `Dvd`, 0x311C bytes; layout unknown).
class cDvd {
public:
    u8 pad_0[0x311C];   // layout unknown; the size keeps `Dvd` out of small data
    // Polls request `req`. Returns 1 when done and then stores the result word, the size and
    // the destination address through the non-NULL pointers; < 0 on failure.
    int ReadCheck(int req, int* result, int* size, void** addr);
    int ReadCheck(int req);
};
extern cDvd Dvd;

extern "C" {
// Queue a file read; returns the request number. `mode` 3 = allocate the destination.
int DvdReadN(const char* name, void* dst, int a, int b, int c, int mode, const char* file, int line);
}

#endif
