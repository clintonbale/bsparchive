#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "bsp.h"
#include "token.h"
#include "archive.h"

#pragma warning(push, 0)  
#include "tinydir.h"
#include "miniz.h"
#pragma warning(pop)

extern hash_table* exclude_table;
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
	if (fmt != NULL) {
		for (int i = 0; i < COUNT_OF(formats); i++) {
			if (strcmp(fmt, formats[i]) == 0) {
				return true;
			}
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

bool normalize_value(char* value) {
	assert(value != NULL);

	while (*value) {
		if(*value & 0x80) {
			printf("Unsupported character found in entity %c - skipping dependency check\n", *value);
			return false;
		}
		else if (isalpha(*value))
			*value = (char)tolower(*value);
		else if (*value == '\\')
			*value = '/';
		value++;
	}
	return true;
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
	static char temp[ENT_MAX_VALUE];
	temp[0] = 0;

	assert(key != NULL);
	assert(value != NULL);
	if (!value[0]) return;

	if (!normalize_value(value)) {
		if(g_verbose) {
			printf("Error normalizing value with key '%s'\n", key);
		}
		return;
	}

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
		if (valid_resource_format(extension)) {
			//TODO: option: add fullpath wad dependencies
			//add_dependency(value);
		}
		char* last_path = strrchr(value, '/');
		if(last_path != NULL) {
			last_path++;
			extension = strrchr(last_path, '.');
			if (valid_resource_format(extension)) {
				add_dependency(last_path);
			}
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
			if (g_verbose) {
				//printf("Unknown file format '%s' - ignoring '%s'\n", extension, value);
			}
		}
	}
}

bool read_dependency(const char* path, void** data, size_t* data_len) {
	bool success = true;
	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		if (g_verbose) {
			printf("Dependency missing: %s\n", path);
		}
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

void add_base_dependencies(const char* bspname) {
	char temp[MAX_PATH];

	sprintf(temp, "maps/%s.bsp", bspname);
	add_dependency(temp);
	sprintf(temp, "maps/%s.txt", bspname);
	add_dependency(temp);
	sprintf(temp, "maps/%s.res", bspname);
	add_dependency(temp);

	sprintf(temp, "overviews/%s.bmp", bspname);
	add_dependency(temp);
	sprintf(temp, "overviews/%s.tga", bspname);
	add_dependency(temp);
	sprintf(temp, "overviews/%s.txt", bspname);
	add_dependency(temp);
}

void get_bsp_name(const char* bsp_path, char* bspname) {
	size_t path_len = strlen(bsp_path);
	const char* last = &bsp_path[path_len];
	
	while (*--last && last != bsp_path) {
		if (*last == '\\' || *last == '/') {
			last++;
			break;
		}
	}
	if (last == bsp_path)
		return;

	// we only want the bsp name so get rid of the extension
	size_t file_len = path_len - (last - bsp_path) - 4;
	strncpy(bspname, last, file_len);
	bspname[file_len] = 0;
}

int archive_bsp(const char* bsp_path, const char* output_path, const char* gamedir) {
	//TODO: enum on exit statuses
	int rc = EXIT_SUCCESS;
	char bspname[MAX_PATH], archivename[MAX_PATH];

	get_bsp_name(bsp_path, bspname);
	if(g_verbose) {
		printf("Processing map: %s\n", bsp_path);
	}
	else {
		printf("Processing map: %s.bsp\n", bspname);
	}

	add_base_dependencies(bspname);

	char* ents = bsp_open_entities(bsp_path);
	if (!ents) {
		rc = EXIT_FAILURE;
		goto exit;
	}
	stream = ents;
	bsp_read_entities(parse_bsp_ent_value);
	
	strcpy(archivename, bspname);
	strcat(archivename, ".zip");

	chdir(output_path);
	remove(archivename);

	mz_zip_archive archive = { 0 };
	// create the archive
	if(!mz_zip_writer_init_file_v2(&archive, archivename, 0, MZ_BEST_COMPRESSION)) {
		printf("Failed to create zip archive: %s, %s\n", archivename, mz_zip_get_error_string(archive.m_last_error));
		rc = EXIT_FAILURE;
		goto exit;
	}	

	size_t ndeps = buf_len(dependency_list);
	size_t dep_success = 0, dep_missing = 0, dep_skipped = 0;

	for (size_t i = 0; i < ndeps; ++i) {
		char* dep_name = dependency_list[i];
		char* fullpath = get_full_path(dep_name, gamedir);

		void* data = NULL;
		size_t data_len = 0;
		
		if(hash_exists(exclude_table, dep_name)) {
			if(g_verbose) printf("Skipping: %s\n", dep_name);
			dep_skipped++;
		}
		else if (read_dependency(fullpath, &data, &data_len)) {
			if(!mz_zip_writer_add_mem_ex(&archive, dep_name, data, data_len, NULL, 0, MZ_BEST_COMPRESSION, 0, 0)) {
				printf("Error adding file to archive: %s, %s\n", dep_name, mz_zip_get_error_string(archive.m_last_error));
			}
			else {
				dep_success++;
			}
		}
		else {
			dep_missing++;
		}
		if (data) free(data);
	}
	
	mz_bool success;
	if(!(success = mz_zip_writer_finalize_archive(&archive))) {
		printf("Error finalizing archive: %s, %s\n", archivename, mz_zip_get_error_string(archive.m_last_error));
	}
	if(!(success = mz_zip_writer_end(&archive))) {
		printf("Error closing archive: %s, %s\n", archivename, mz_zip_get_error_string(archive.m_last_error));				
	}

	if (success) {
		printf("Archived map '%s' successfully: %u files added, %u could not be found.\n", bspname, dep_success, dep_missing);
	}
	else {
		printf("Failed archiving map '%s'\n", bspname);
		remove(archivename);
		rc = EXIT_FAILURE;
	}	
exit:
	stream = NULL;
	if (ents) free(ents);
	if (dependency_list) {
		for(size_t i = 0; i < ndeps; ++i) {
			if(dependency_list[i]) {
				free(dependency_list[i]);
				dependency_list[i] = NULL;
			}
		}
		buf_clear(dependency_list);
	}
	return rc;
}
