#ifndef FILE_H
#define FILE_H

#include "types.h"

// game/file.cpp: host file access through the SN file server, enabled by pG->flags_54 & 0x20000.
extern "C" {
int InitFile();
int file_open(const char* name, int mode);   // 0/2: create, 1/3: open read-only / read-write
int file_read(int fd, void* buf, int size);
int file_write(int fd, const void* buf, int size);
int file_seek(int fd, int offset, int whence);
int file_exist(const char* name);
int file_path(const char* dir);
}

#endif
