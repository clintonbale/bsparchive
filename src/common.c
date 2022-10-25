#pragma once
#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#pragma warning(push, 0)  
#include "tinydir.h"
#pragma warning(pop)

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

//FNV-1
static uint64_t hash(const char* data) {
	uint64_t hash = 0xCBF29CE484222325;
	for(size_t i = 0; data[i]; ++i) {
		hash = hash * 0x100000001B3;
		hash = hash ^ data[i];
	}
	return hash;
}

hash_table* hashtable_create(size_t items) {
	hash_table* map = xcalloc(1, sizeof(hash_table));

	items = (max(items*2, 16) + 0x1F) & ~0x1F;

	map->vals = xcalloc(items, sizeof(const char*));
	map->cap = items;

	return map;
}

void hashtable_add(hash_table* ht, const char* data) {
	assert(ht != NULL);
	assert(data);
	assert(ht->len < ht->cap);

	uint64_t index = hash(data) % ht->cap;
	
	while (ht->vals[index] != NULL) {
		++index;

		index %= ht->cap;
	}

	ht->vals[index] = data;
	ht->len++;
}

bool hashtable_contains(hash_table* ht, const char* data) {
	assert(ht != NULL);
	assert(data);

	uint64_t index = hash(data) % ht->cap;
	
	while (ht->vals[index] != NULL) {
		if (strcmp(ht->vals[index], data) == 0)
			return true;

		++index;
		index %= ht->cap;
	}

	return false;
}

bool is_valid_file(const char* filepath) {
	assert(filepath != NULL);
	bool valid = false;

	FILE* file = fopen(filepath, "r");
	if (file != NULL) {
		fclose(file);
		return true;
	}
	return false;
}

bool is_valid_dir(const char* path) {
	assert(path != NULL);
	bool valid = false;

	tinydir_dir dir;
	tinydir_open(&dir, path);

	valid = dir.has_next > 0;

	tinydir_close(&dir);
	return valid;
}