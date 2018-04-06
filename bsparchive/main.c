#define _CRT_SECURE_NO_DEPRECATE

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argtable3.h"

#include "common.h"
#include "token.h"
#include <ctype.h>

#include "bsp.h"
#include "tinydir.h"

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
			printf("Unknown file format '%s' - ignoring '%s'\n", extension, value);
		}
	}
}

char** deps = NULL;

void add_dependency(char* value) {
	for(int i = 0; i < buf_len(deps); ++i) {
		if (strcmp(deps[i], value) == 0)
			return;
	}
	buf_push(deps, strdup(value));

	printf("dependency: %s\n", value);
}

void do_bsp(char* bsp_path) {
	bspheader* bsp = bsp_open(bsp_path);
	assert(bsp->version == 30);
	
	stream = (char*)((char*)bsp + bsp->lump[LUMP_ENTITIES].offset);
	bsp_read_entities(parse_bsp_ent_value);

	size_t len = buf_len(deps);
	for (int i = 0; i < len; ++i) {
		char* dependency = deps[i];
		// TODO: ...
	}

	buf_clear(deps);
	free(bsp);
}

bool verbose = false;


struct arg_lit *verb, *help, *ver;
struct arg_file *gamedir, *file, *output;
struct arg_end *end;

int main(int argc, char* argv[]) {
	const char* progname = "bsparchive";
	const char* version = "0.1";

	token_test();

	void *argtable[] = {
		help = arg_litn("h", "help", 0, 1, "print this help and exit"),
		verb = arg_litn("v", "verbose", 0, 1, "verbose output"),
		ver = arg_litn("V", "version", 0, 1, "print version information and exit"),
		gamedir = arg_filen("g", "gamedir", "<PATH>", 0, 1, "the game directory"),
		output = arg_filen("o", "output", "<PATH>", 1, 1, "where to output the zip files"),
		file = arg_filen(NULL, NULL, "<PATH>", 1, 100, "bsp files or map directories"),
		end = arg_end(20),
	};

	int exitcode;

	int nerrors;
	nerrors = arg_parse(argc, argv, argtable);

	if (help->count > 0)
	{
		printf("%s %s\nUsage: %s", progname, version, progname);
		arg_print_syntax(stdout, argtable, "\n");
		printf("Identifies and archives all dependencies for bsp files.\n\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		
		exitcode = 0;
		goto exit;
	}

	if(ver->count > 0) {
		printf("%s %s\n", progname, version);
		exitcode = 0;
		goto exit;
	}
	

	for (int i = 0; i < file->count; i++) {
		printf("file = %s\n", file->filename[i]);
		do_bsp(file->filename[i]);
	}
	for (int i = 0; i < output->count; i++) {
		printf("out = %s\n", output->filename[i]);
	}

	//do_bsp(argv[1]);

	//getchar();
	if (nerrors > 0) {
		arg_print_errors(stdout, end, progname);
		printf("Try '%s --help' for more information.\n", progname);
		exitcode = 1;
		goto exit;
	}

exit:
	arg_freetable(argtable, COUNT_OF(argtable));
	getchar();
	return exitcode;
}