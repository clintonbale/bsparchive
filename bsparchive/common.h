#pragma once
#include <stddef.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

void fatal(char* fmt, ...);
void* xmalloc(size_t size);