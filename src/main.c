#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "archive.h"
#include "common.h"

#pragma warning(push, 0)  
#include "argtable3.h"
#include "tinydir.h"
#pragma warning(pop)

bool g_verbose;
hash_table* exclude_table;

static struct arg_lit *a_verbose, *a_help, *a_version;
static struct arg_file *a_gamedir, *a_file, *a_output;
static struct arg_end *end;

static const char* const exclude_list[] = {	
	#include "../res/goldsrc-manifest.lst"
};

static bool is_valid_dir(const char* path) {
	assert(path != NULL);
	bool valid = false;

	tinydir_dir dir;
	tinydir_open(&dir, path);

	valid = dir.has_next > 0;

	tinydir_close(&dir);
	return valid;
}

static bool is_valid_file(const char* filepath) {
	assert(filepath != NULL);
	bool valid = false;

	FILE* file = fopen(filepath, "r");
	if(file != NULL) {
		fclose(file);
		return true;
	}
	return false;	
}

static const char* gamedir_folders[] = {
	"sound",
	"gfx",
	"sprites",
	"models",
	"maps",
};

#define GAMEDIR_PARENT_SEARCH_DEPTH 3
#define GAMEDIR_VALID_MIN 2

static bool is_valid_gamedir(const char* gamedir) {
	tinydir_dir dir;
	if (tinydir_open(&dir, gamedir) != 0)
		return false;

	size_t matches = 0;

	while (dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (file.is_dir) {
			for (int i = 0; i < COUNT_OF(gamedir_folders); i++) {
				if (strcasecmp(file.name, gamedir_folders[i]) == 0) {
					matches++;
				}
			}
		}
		tinydir_next(&dir);
	}

	return matches > GAMEDIR_VALID_MIN;
}

static void get_parent_dir(char* input) {
	size_t len = strlen(input);
	char* last = (char*)(input + len);
	while (last != input && *last != '\\' && *last != '/') {
		last--;
	}
	input[last - input] = 0;
}

static const char* find_gamedir(const char* input, bool is_input_dir) {
	static char gamedir[MAX_PATH];
	strcpy(gamedir, input);

	size_t depth = 0;

	while(depth++ < GAMEDIR_PARENT_SEARCH_DEPTH && gamedir[0] && !is_valid_gamedir(gamedir)) {		
		get_parent_dir(gamedir);
	}

	return gamedir;
}

void load_exclude_manifest() {
	size_t items = COUNT_OF(exclude_list);

	exclude_table = hashtable_create(items);
	assert(exclude_table != NULL);
	
	for(size_t i = 0; i < items; ++i) {
		hashtable_add(exclude_table, exclude_list[i]);
	}

	assert(hashtable_contains(exclude_table, "sound/hgrunt/fire!.wav"));
	assert(hashtable_contains(exclude_table, "sprites/fog5.spr"));
	assert(hashtable_contains(exclude_table, "gfx/vgui/fonts/800_title font.tga"));
}

int main(int argc, char* argv[]) {
	const char* progname = "bsparchive";
	const char* version = "0.1";
	
	//TODO: option to output list of missing dependencies only
	//TODO: option to skip checking exclusion list and include everything
	//TODO:		exclusion list per game mod type.

	void *argtable[] = {
		a_help = arg_litn("h", "help", 0, 1, "print this help and exit"),
		a_verbose = arg_litn("v", "verbose", 0, 1, "verbose output"),
		a_version = arg_litn("V", "version", 0, 1, "print version information and exit"),
		a_gamedir = arg_filen("g", "gamedir", "<PATH>", 0, 1, "the game directory"),
		a_output = arg_filen("o", "output", "<PATH>", 1, 1, "where to output the zip files"),
		a_file = arg_filen(NULL, NULL, "<PATH>", 1, 1, "bsp files or map directories"),
		end = arg_end(20),
	};

	int rc;

	int nerrors;
	nerrors = arg_parse(argc, argv, argtable);

	if (a_help->count > 0)
	{
		printf("%s %s\nUsage: %s", progname, version, progname);
		arg_print_syntax(stdout, argtable, "\n");
		printf("Identifies and archives all dependencies for bsp files.\n\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		
		rc = EXIT_SUCCESS;
		goto exit;
	}

	if(a_version->count > 0) {
		printf("%s %s\n", progname, version);
		rc = EXIT_SUCCESS;
		goto exit;
	}
			
	if (nerrors > 0) {
		arg_print_errors(stdout, end, progname);
		printf("Try '%s --help' for more information.\n", progname);
		rc = EXIT_FAILURE;
		goto exit;
	}

	g_verbose = a_verbose->count > 0;

	assert(a_file->count == 1);
	assert(a_output->count == 1);
	assert(a_gamedir->count <= 1);

	const char* input = a_file->filename[0];
	const char* output = a_output->filename[0];
	const char* gamedir = NULL;

	if (!is_valid_dir(output)) {
		printf("Missing or invalid output directory location: %s\n", output);
		rc = EXIT_FAILURE;
		goto exit;
	}

	bool is_input_dir = false;

	if (is_valid_dir(input)) {
		is_input_dir = true;
	} else {
		if(strncmp(a_file->extension[0], ".bsp", 3) != 0) {
			printf("Invalid file: %s\nOnly .bsp files supported for archival.", input);
			rc = EXIT_FAILURE;
			goto exit;
		}
		if (!is_valid_file(input)) {
			printf("Invalid or missing file %s\n", input);
			rc = EXIT_FAILURE;
			goto exit;
		}
	}
	
	if (a_gamedir->count == 1) {
		gamedir = a_gamedir->filename[0];
		if (!is_valid_dir(gamedir)) {
			printf("Game directory could not be found %s\n", gamedir);;
			rc = EXIT_FAILURE;
			goto exit;
		}
	}
	else if(a_gamedir->count == 0) {
		gamedir = find_gamedir(input, is_input_dir);
		if(!gamedir) {
			printf("Missing or invalid game directory, the game directory is the mod folder for your Half-Life game.\n");
			rc = EXIT_FAILURE;
			goto exit;
		}
	}
	
	if(!is_valid_gamedir(gamedir)) {
		printf("Could not find valid game directory.");
		rc = EXIT_FAILURE;
		goto exit;
	}

	if(g_verbose) {
		printf("Game directory: %s\n", gamedir);
	}

	load_exclude_manifest();

	if(is_input_dir) {
		rc = archive_bsp_dir(input, output, gamedir);
	}
	else {
		rc = archive_bsp(input, output, gamedir);
	}

exit:
	arg_freetable(argtable, COUNT_OF(argtable));
	return rc;
}
