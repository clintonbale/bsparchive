#pragma once
#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

void* xcalloc(size_t count, size_t size) {
	void* ptr = calloc(count, size);
	if (!ptr) {
		perror("xcalloc failed");
		exit(1);
	}
	return ptr;
}

void *xrealloc(void *ptr, size_t num_bytes) {
	ptr = realloc(ptr, num_bytes);
	if (!ptr) {
		perror("xrealloc failed");
		exit(1);
	}
	return ptr;
}

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
	assert(buf_cap(buf) <= (SIZE_MAX - 1) / 2);
	size_t new_cap = max(16, max(1 + 2 * buf_cap(buf), new_len));
	assert(new_len <= new_cap);
	assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf)) / elem_size);
	size_t new_size = offsetof(BufHdr, buf) + new_cap * elem_size;
	BufHdr *new_hdr;
	if (buf) {
		new_hdr = xrealloc(buf__hdr(buf), new_size);
	}
	else {
		new_hdr = xmalloc(new_size);
		new_hdr->len = 0;
	}
	new_hdr->cap = new_cap;
	return new_hdr->buf;
}

size_t hash(const char* data) {
	char* b = data;
	size_t x = 0xcbf29ce484222325;
	while(*b) {
		x ^= *b++;
		x *= 0x100000001b3;
		x ^= x >> 32;
	}
	return x;
}

hash_table* hash_create(size_t items) {
	hash_table* map = xcalloc(1, sizeof(hash_table));

	items = max(items*2, 16);

	map->vals = xcalloc(items, sizeof(const char*));
	map->cap = items;

	return map;
}

void hash_add(hash_table* ht, const char* data) {
	assert(ht != NULL);
	assert(data);
	assert(ht->len < ht->cap);

	size_t index = hash(data) % ht->cap;
	
	while (ht->vals[index] != NULL) {
		++index;

		index %= ht->cap;
	}

	ht->vals[index] = data;
	ht->len++;
}

bool hash_exists(hash_table* ht, const char* data) {
	assert(ht != NULL);
	assert(data);

	size_t index = hash(data) % ht->cap;
	
	while (ht->vals[index] != NULL) {
		if (strcmp(ht->vals[index], data) == 0)
			return true;

		++index;
		index %= ht->cap;
	}

	return false;
}