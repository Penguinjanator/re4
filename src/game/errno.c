/* newlib 1.8.2 libc/errno/errno.c */
#include "newlib_stdio.h"

int *__errno()
{
    return &_REENT->_errno;
}
