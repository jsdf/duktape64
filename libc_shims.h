#ifndef _LIBC_SHIMS_H
#define _LIBC_SHIMS_H 1

#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// defined in /usr/include/PR/os_libc.h
extern int sprintf(char*, const char*, ...);
// defined in kmc/stdio.h
// extern int sscanf(const char *,const char *, ...);

extern void ed64PrintfSync2(const char* fmt, ...);

#define printf ed64PrintfSync2

void abort();

int vsprintf(char* s, const char* fmt, va_list ap);

int vsnprintf(char* s, size_t n, const char* format, va_list ap);

int vasprintf(char** result, const char* format, va_list args);

int snprintf(char* s, size_t n, const char* format, ...);

int sscanf(const char* str, const char* format, ...);

#endif /* _LIBC_SHIMS_H  */
