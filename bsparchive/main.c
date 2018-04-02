#define _CRT_SECURE_NO_DEPRECATE

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "token.h"
#include <ctype.h>

typedef struct bsplump {
	int32_t offset;
	int32_t length;
} bsplump;

typedef struct bspheader {
	int32_t version;
	bsplump lump[15];
} bspheader;

#define LUMP_ENTITIES 0
#define ENT_MAX_KEY 32
#define ENT_MAX_VALUE 1024

bspheader* bsp_open(char* path) {
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

void bsp_read_ent_value(char* key, char* value);
void bsp_read_ent_values(char* key, char* value);

void bsp_get_dependencies() {
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

			bsp_read_ent_values(key, value);
			next_token();
		}
		expect_token(TOKEN_END_ENT);
	}
}

void bsp_read_ent_values(char* key, char* value) {
	char* last = value;
	char* end = strchr(value, ';');
	if (end) { 
		// multiple values potentially delimited by semicolon
		do { 
			size_t len = end - last;
			last[len] = 0;
			bsp_read_ent_values(key, last);

			last = last + len + 1;
			end = strchr(last, ';');
		} while (end);
		bsp_read_ent_value(key, last);
	}
	else { 
		// a single value
		bsp_read_ent_value(key, value);
	}
}

void normalize_path(char* file_path) {
	assert(file_path != NULL);

	while(*file_path) {
		if (isalpha(*file_path))
			*file_path = tolower(*file_path);
		else if(*file_path == '\\')
			*file_path = '/';
		file_path++;
	}
}

const char* formats[] = {
	".mdl",
	".wav",
	".spr",
	".wad",
	".tga",
	".bmp",
	".txt",
};

bool valid_format(const char* fmt) {
	for (int i = 0; i < COUNT_OF(formats); i++) {
		if (strcmp(fmt, formats[i]) == 0) {
			return true;
		}
	}
	return false;
}

void add_dependency(char* value) {
	normalize_path(value);

	// todo: copy value to new list
	// todo: duplicate detection
	printf("dependency: %s\n", value);
}

const char* gfx_sides[] = {
	"up.tga",
	"dn.tga",
	"lf.tga",
	"rt.tga",
	"ft.tga",
	"bk.tga"
};

void bsp_read_ent_value(char* key, char* value) {
	assert(key != NULL);
	assert(value != NULL);

	char* extension = strrchr(value, '.');
	if(strcmp(key, "skyname") == 0) {
		char buf[1024] = "gfx/env/";
		strcat(buf, value);

		size_t len = strlen(buf), side_len = COUNT_OF(gfx_sides);
		while(side_len--) {
			buf[len] = 0;
			strcat(buf, gfx_sides[side_len]);
			add_dependency(buf);
		}
	}
	else if(strcmp(key, "wad") == 0) {
		// ...
		normalize_path(value);
		printf("wad: %s\n", value);
	}
	else if(extension) {
		if (valid_format(extension)) {
			add_dependency(value);
		}
		else {
			// ...
		}
	}
}

int main(int argc, char* argv[]) {
	token_test();

	bspheader* bsp = bsp_open(argv[1]);
	assert(bsp->version == 30);

	stream = (char*)((char*)bsp + bsp->lump[LUMP_ENTITIES].offset);
	bsp_get_dependencies();

	free(bsp);
	getchar();
	return 0;
}