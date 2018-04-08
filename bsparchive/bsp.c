#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"
#include "common.h"
#include "token.h"

char* bsp_open_entities(const char* path) {
	assert(path != NULL);
	char* entities = NULL;

	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		perror("Error opening bsp");
		goto exit;
	}

	bspheader header;
	if(fread(&header, sizeof(bspheader), 1, fp) != 1) {
		perror("Error reading bsp header");
		goto exit;
	};
	// only gold source supported
	assert(header.version == 30);

	bsplump entities_lump = header.lump[LUMP_ENTITIES];
	assert(entities_lump.offset > 0);
	assert(entities_lump.length > 0);

	entities = (char*)xmalloc(entities_lump.length);

	fseek(fp, entities_lump.offset, SEEK_SET);
	if (fread(entities, sizeof(char), entities_lump.length, fp) != entities_lump.length) {
		perror("Error reading bsp file");
		free(entities);
		entities = NULL;	
	}
	
exit:
	if(fp) fclose(fp);
	return entities;
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