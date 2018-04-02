#pragma once
#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal(char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("FATAL: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	exit(1);
}

void* xmalloc(size_t size) {
	void* ptr = malloc(size);
	if (!ptr) {
		perror("xmalloc failed");
		exit(1);
	}
	return ptr;
}