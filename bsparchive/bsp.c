#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"
#include "common.h"
#include "token.h"

bspheader* bsp_open(const char* path) {
	assert(path != NULL);

	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		perror("Error opening bsp");
		exit(1);
	}

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	rewind(fp);

	char* buf = xmalloc(file_size * sizeof(char));
	size_t size_read = fread(buf, sizeof(char), file_size, fp);
	assert(size_read == file_size);

	fclose(fp);
	return (bspheader*)buf;
}

static void bsp_read_ent_values(const bsp_entity_reader reader, char* key, char* value) {
	char* last = value;
	char* end = strchr(value, ';');
	if (end) {
		// multiple values potentially delimited by semicolon
		do {
			size_t len = end - last;
			last[len] = 0;
			bsp_read_ent_values(reader, key, last);

			last = last + len + 1;
			end = strchr(last, ';');
		} while (end);

		reader(key, last);
	}
	else {
		// a single value
		reader(key, value);
	}
}

void bsp_read_entities(bsp_entity_reader reader) {
	assert(reader != NULL);

	char key[ENT_MAX_KEY];
	char value[ENT_MAX_VALUE];

	next_token();
	while (match_token(TOKEN_BEGIN_ENT)) {
		while (is_token(TOKEN_STR)) {
			strncpy(key, token.start, token.end - token.start);
			key[token.end - token.start] = 0;

			expect_token(TOKEN_STR);

			strncpy(value, token.start, token.end - token.start);
			value[token.end - token.start] = 0;

			bsp_read_ent_values(reader, key, value);
			next_token();
		}
		expect_token(TOKEN_END_ENT);
	}
}