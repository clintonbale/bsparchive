#pragma once
#include <stdbool.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#ifdef _WIN32
#include <windows.h>
#define strcasecmp _stricmp
#define chdir SetCurrentDirectory
#endif

void fatal(char* fmt, ...);
void* xmalloc(size_t size);
void* xcalloc(size_t count, size_t size);
void* xrealloc(void *ptr, size_t num_bytes);

typedef struct BufHdr {
	size_t len;
	size_t cap;
	char buf[];
} BufHdr;

#define buf__hdr(b) ((BufHdr *)((char *)(b) - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b)*sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_fit(b, n) ((n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...) (buf_fit((b), 1 + buf_len(b)), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

void *buf__grow(const void* buf, size_t new_len, size_t elem_size);

typedef struct map {
	const char** vals;
	size_t len;
	size_t cap;
} hash_table;

hash_table* hashtable_create(size_t initial_size);
void hashtable_add(hash_table* ht, const char* data);
bool hashtable_contains(hash_table* ht, const char* data);