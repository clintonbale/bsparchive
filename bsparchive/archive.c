#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "bsp.h"
#include "token.h"
#include "lib/tinydir.h"
#include "archive.h"


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

void add_dependency(char* value);


void normalize_value(char* file_path) {
	assert(file_path != NULL);

	while (*file_path) {
		if (isalpha(*file_path))
			*file_path = tolower(*file_path);
		else if (*file_path == '\\')
			*file_path = '/';
		file_path++;
	}
}

void parse_bsp_ent_value(char* key, char* value) {
	assert(key != NULL);
	assert(value != NULL);
	if (!value[0]) return;

	normalize_value(value);

	char buf[1024] = "";

	char* extension = strrchr(value, '.');
	if (strcmp(key, "skyname") == 0) {
		strcat(buf, "gfx/env/");
		strcat(buf, value);

		size_t len = strlen(buf), side_len = COUNT_OF(gfx_sides);
		while (side_len--) {
			buf[len] = 0;
			strcat(buf, gfx_sides[side_len]);
			add_dependency(buf);
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
				strcat(buf, "sound/");
				strcat(buf, value);
				add_dependency(buf);
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

char** deps = NULL;

void add_dependency(char* value) {
	for (int i = 0; i < buf_len(deps); ++i) {
		if (strcmp(deps[i], value) == 0)
			return;
	}
	buf_push(deps, strdup(value));
	if (g_verbose) {
		printf("Found dependency: %s\n", value);
	}
}

bool do_bsp(char* bsp_path) {
	char* ents = bsp_open_entities(bsp_path);
	if(!ents) {
		goto exit;
	}
	stream = ents;	
	bsp_read_entities(parse_bsp_ent_value);
	stream = NULL;

	free(ents);

	size_t len = buf_len(deps);
	for (int i = 0; i < len; ++i) {
		char* dependency = deps[i];

		free(dependency);
	}

exit:
	if(deps) buf_clear(deps);
}

int archive_bsp_dir(const char* input, const char* output, const char* gamedir) {	
	tinydir_dir dir;
	tinydir_file file;
	if(tinydir_open(&dir, input) != 0) {
		printf("Error opening directory %s\n", input);
		return EXIT_FAILURE;
	}

	assert(dir.has_next > 0);

	printf("Archiving map directory %s\n", input);

	while(dir.has_next) {
		tinydir_readfile(&dir, &file);
		if (strncmp(file.extension, "bsp", 3) == 0) {
			archive_bsp(file.path, output, gamedir);
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);
	
	return EXIT_SUCCESS;
}

int archive_bsp(const char* bsp_path, const char* output_path, const char* gamedir) {
	printf("Processing: %s\n", bsp_path);;
	
	do_bsp(bsp_path);

	return EXIT_SUCCESS;
}