#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "bsp.h"
#include "token.h"
#include "archive.h"

#pragma warning(push, 0)  
#include "../ext/tinydir.h"
#include "../ext/miniz.h"
#pragma warning(pop)

static char** dependency_list = NULL;

const char* const formats[] = {
	".mdl",
	".wav",
	".spr",
	".wad",
	".tga",
	".bmp",
	".txt",
};
const char* const gfx_sides[] = {
	"up.tga",
	"dn.tga",
	"lf.tga",
	"rt.tga",
	"ft.tga",
	"bk.tga"
};
const char* const speak_keys[] = {
	"speak",
	"team_speak",
};

bool valid_resource_format(const char* fmt) {
	for (int i = 0; i < COUNT_OF(formats); i++) {
		if (strcmp(fmt, formats[i]) == 0) {
			return true;
		}
	}
	return false;
}

bool is_speak_key(const char* key) {
	for (int i = 0; i < COUNT_OF(speak_keys); i++) {
		if (strcmp(key, speak_keys[i]) == 0) {
			return true;
		}
	}
	return false;
}

void normalize_value(char* value) {
	assert(value != NULL);

	while (*value) {
		if (isalpha(*value))
			*value = (char)tolower(*value);
		else if (*value == '\\')
			*value = '/';
		value++;
	}
}

void add_dependency(char* value) {
	for (size_t i = 0; i < buf_len(dependency_list); ++i) {
		if (strcmp(dependency_list[i], value) == 0)
			return;
	}
	buf_push(dependency_list, _strdup(value));
	if (g_verbose) {
		//TODO: log where the dependency is from (the bsp)
		printf("Found dependency: %s\n", value);
	}
}

void parse_bsp_ent_value(char* key, char* value) {
	static char temp[1024];
	temp[0] = 0;

	assert(key != NULL);
	assert(value != NULL);
	if (!value[0]) return;

	normalize_value(value);

	char* extension = strrchr(value, '.');
	if (strcmp(key, "skyname") == 0) {
		strcat(temp, "gfx/env/");
		strcat(temp, value);

		size_t len = strlen(temp), side_len = COUNT_OF(gfx_sides);
		while (side_len--) {
			temp[len] = 0;
			strcat(temp, gfx_sides[side_len]);
			add_dependency(temp);
		}
	}
	else if (strcmp(key, "wad") == 0) {
		add_dependency(value);
		char* last_path = strrchr(value, '/');
		if(last_path != NULL) {
			last_path++;
			add_dependency(last_path);
		}
	}
	else if (is_speak_key(key)) {
		// TODO: handle speak sentences
		printf("Speak sentence: %s", value);
	}
	else if (extension) {
		if (valid_resource_format(extension)) {
			if (strcmp(extension, ".wav") == 0) {
				strcat(temp, "sound/");
				strcat(temp, value);
				add_dependency(temp);
			}
			else {
				add_dependency(value);
			}
		}
		else if (strlen(extension) >= 4) {
			// if verbose
			if (g_verbose) {
				printf("Unknown file format '%s' - ignoring '%s'\n", extension, value);
			}
		}
	}
}

bool read_dependency(const char* path, void** data, size_t* data_len) {
	bool success = true;
	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		printf("Dependency missing: %s\n", path);
		//*data = NULL;
		//*data_len = 0;
		success = false;
		goto exit;
	}
	
	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	*data = (void*)xmalloc(size + 1);
	*data_len = size;

	if(fread((void*)*data, sizeof(char), size, fp) != size) {
		printf("Error reading file %s\n", path);
		*data = NULL;
		*data_len = 0;
		success = false;
		goto exit;
	}
exit:
	if (fp) fclose(fp);
	return success;
}


char* get_full_path(const char* dependency, const char* gamedir) {
	static char full_path[MAX_PATH];
	full_path[0] = 0;
	
	strcat(full_path, gamedir);
	if (*dependency != '/' && *dependency != '\\') {
		strcat(full_path, "/");
	}
	strcat(full_path, dependency);

	assert(strlen(full_path) < MAX_PATH);

	return full_path;
}

int archive_bsp_dir(const char* input, const char* output, const char* gamedir) {
	//TODO: enum on exit statuses
	tinydir_dir dir;
	tinydir_file file;
	if (tinydir_open(&dir, input) != 0) {
		printf("Error opening directory %s\n", input);
		return EXIT_FAILURE;
	}

	assert(dir.has_next > 0);

	printf("Archiving map directory %s\n", input);

	while (dir.has_next) {
		tinydir_readfile(&dir, &file);
		if (strncmp(file.extension, "bsp", 3) == 0) {
			archive_bsp(file.path, output, gamedir);
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);

	return EXIT_SUCCESS;
}

void add_extra_dependencies(const char* bsp_path) {	
	//TODO: add .bsp, .txt and .res files
	// maps/mapname.bsp
	// maps/mapname.txt
	// maps/mapname.res
	//TODO: add overviews:
	// overviews/mapname.bmp
	// overviews/mapname.tga
	// overviews/mapname.txt
}

int archive_bsp(const char* bsp_path, const char* output_path, const char* gamedir) {
	//TODO: enum on exit statuses
	int rc = EXIT_SUCCESS;
	printf("Processing: %s\n", bsp_path);;

	char* ents = bsp_open_entities(bsp_path);
	if (!ents) {
		rc = EXIT_FAILURE;
		goto exit;
	}
	stream = ents;
	bsp_read_entities(parse_bsp_ent_value);

	add_extra_dependencies(bsp_path);
	
	size_t len = buf_len(dependency_list);
	for (size_t i = 0; i < len; ++i) {
		char* dep_name = dependency_list[i];
		char* fullpath = get_full_path(dep_name, gamedir);

		void* data = NULL;
		size_t data_len = 0;

		if (read_dependency(fullpath, &data, &data_len)) {
			printf("read the file! %s - %u\n", fullpath, data_len);
			//TODO: zip it n ship it.
		}
	next:
		if (data) free(data);
		if (dependency_list[i]) {
			free(dependency_list[i]);
			dependency_list[i] = NULL;
		}
	}
exit:
	stream = NULL;
	if (ents) free(ents);
	if (dependency_list) buf_clear(dependency_list);
	return rc;
}
