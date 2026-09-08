/* gcc 2.95 ginclude/va-ppc.h (System V.4): va_list for ProDG-compiled game and libc code. */
#ifndef VA_PPC_H
#define VA_PPC_H

typedef struct __va_list_tag {
    unsigned char gpr;
    unsigned char fpr;
    char *overflow_arg_area;
    char *reg_save_area;
} __va_list[1], __gnuc_va_list[1];
typedef __gnuc_va_list va_list;

#ifdef __cplusplus
/* g++ 2.95 does not inline __builtin_memcpy; the struct copy gives the original 3-word copy */
#define va_start(AP, LASTARG) \
    (__builtin_next_arg(LASTARG), *(AP) = *(struct __va_list_tag*) __builtin_saveregs())
#else
#define va_start(AP, LASTARG) \
    (__builtin_next_arg(LASTARG), __builtin_memcpy((AP), __builtin_saveregs(), sizeof(__gnuc_va_list)))
#endif
#define va_end(AP) ((void)0)

#endif
