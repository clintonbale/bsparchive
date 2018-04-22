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

static char bspname[MAX_PATH] = {0};

static const char* const formats[] = {
	".mdl",
	".wav",
	".spr",
	".wad",
	".tga",
	".bmp",
	".txt",
};
static const char* const gfx_sides[] = {
	"up.tga",
	"dn.tga",
	"lf.tga",
	"rt.tga",
	"ft.tga",
	"bk.tga"
};
static const char* const speak_keys[] = {
	"speak",
	"team_speak",
	"non_team_speak",
	"owners_team_speak",
	"non_owners_team_speak",
	"ap_speak",
	"mpg_speak",
};

bool valid_resource_format(const char* fmt) {
	if (fmt != NULL) {
		for (int i = 0; i < COUNT_OF(formats); i++) {
			if (strcasecmp(fmt, formats[i]) == 0) {
				return true;
			}
		}
	}
	return false;
}

bool is_speak_key(const char* key) {
	for (int i = 0; i < COUNT_OF(speak_keys); i++) {
		if (strcasecmp(key, speak_keys[i]) == 0) {
			return true;
		}
	}
	return false;
}

bool normalize_value(char* value) {
	assert(value != NULL);

	while (*value) {
		if(*value & 0x80) {
			if (g_verbose) {
				printf("Unsupported character found in entity %c - skipping dependency check\n", *value);
			}
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

void add_dependency(const char* value) {
	for (size_t i = 0; i < buf_len(dependency_list); ++i) {
		if (strcmp(dependency_list[i], value) == 0)
			return;
	}
	buf_push(dependency_list, strdup(value));
	if (g_verbose) {
		printf("[%s.bsp] dependency: %s\n", bspname, value);
	}
}

void free_dependency_list() {
	if (dependency_list) {
		size_t ndeps = buf_len(dependency_list);
		for (size_t i = 0; i < ndeps; ++i) {
			if (dependency_list[i]) {
				free(dependency_list[i]);
				dependency_list[i] = NULL;
			}
		}
		buf_clear(dependency_list);
	}
}

void parse_sentence(const char* sentence) {
	assert(sentence != NULL);
	if (sentence && sentence[0] != '!' && sentence[0] != '#') {
		char dep_path[MAX_PATH] = "sound/";

		size_t sentence_len = strlen(sentence);
		if (strstr(sentence, ".wav") - sentence == sentence_len - 4) {
			add_dependency(sentence);
			return;
		}

		char* start = strchr(sentence, '(');
		while (start) {
			char* end = strchr(start, ')');
			if (end) {
				memset(start, ' ', end - start + 1);
			}
			start = strchr(end, '(');
		}
		
		char* slash = strchr(sentence, '/');
		if (slash) {
			strncat(dep_path, sentence, slash - sentence + 1);
			start = slash + 1;
		}
		else {
			strcat(dep_path, "vox/");
			start = sentence;
		}

		size_t spn = strcspn(start, "\t\r\n<>:\"\\/|?*,!()'");
		while (start[spn]) {
			start[spn] = ' ';
			spn = strcspn(start, "\t\r\n.<>:\"\\/|?*,!()'");
		}

		size_t end_dep_len = strlen(dep_path);

		char* token = strtok(start, " ");
		while (token) {
			dep_path[end_dep_len] = 0;
			strcat(dep_path, token);
			strcat(dep_path, ".wav");
			add_dependency(dep_path);
			token = strtok(NULL, " ");
		}
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
	if (strcasecmp(key, "skyname") == 0) {
		strcat(temp, "gfx/env/");
		strcat(temp, value);

		size_t len = strlen(temp), side_len = COUNT_OF(gfx_sides);
		while (side_len--) {
			temp[len] = 0;
			strcat(temp, gfx_sides[side_len]);
			add_dependency(temp);
		}
	}
	else if (strcasecmp(key, "wad") == 0) {
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
		parse_sentence(value);
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
	}
}

bool read_dependency(const char* path, void** data, size_t* data_len) {
	bool success = true;
	FILE* fp = fopen(path, "rb");
	if (fp == NULL) {
		if (g_verbose) {
			printf("[%s.bsp] missing dependency: %s\n", bspname, path);
		}
		success = false;
		goto exit;
	}
	
	fseek(fp, 0, SEEK_END);
	const size_t size = ftell(fp);
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

bool get_bsp_name(const char* bsp_path, char* bspname) {
	size_t path_len = strlen(bsp_path);
	const char* last = &bsp_path[path_len];
	
	while (*--last && last != bsp_path) {
		if (*last == '\\' || *last == '/') {
			last++;
			break;
		}
	}
	if (last == bsp_path)
		return false;

	// we only want the bsp name so get rid of the extension
	size_t file_len = path_len - (last - bsp_path) - 4;
	strncpy(bspname, last, file_len);
	bspname[file_len] = 0;
	return true;
}

int bsp_get_deps(const char* bsp_path, const char* bspname) {
	free_dependency_list();
	int rc = EXIT_SUCCESS;
	add_base_dependencies(bspname);

	char* ents = bsp_open_entities(bsp_path);
	if (!ents) {
		rc = EXIT_FAILURE;
		goto exit;
	}
	stream = ents;
	bsp_read_entities(parse_bsp_ent_value);
exit:
	stream = NULL;
	if(ents) free(ents);
	return rc;
}

int archive_print_deps(const char* bsp_path) {
	if(!get_bsp_name(bsp_path, bspname)) {
		printf("Error getting bsp name from path %s", bsp_path);
		return EXIT_FAILURE;
	}
	bsp_get_deps(bsp_path, bspname);

	printf("// %s.res generated by bsparchive (https://github.com/clintonbale/bsparchive)\n", bspname);

	const size_t ndeps = (size_t)buf_len(dependency_list);
	for (size_t i = 0; i < ndeps; ++i) {
		const char* dep = dependency_list[i];

		if(!g_noexclude && hashtable_contains(exclude_table, dep)) {
			printf("// %s\n", dep);
		}
		else {
			printf("%s\n", dep);
		}
	}

	printf("// %s.bsp - %llu total dependencies", bspname, ndeps);

	free_dependency_list();
	return EXIT_SUCCESS;
}

int archive_bsp_dir(const char* input_dir, const char* output_path, const char* gamedir) {
	//TODO: enum on exit statuses
	tinydir_dir dir;
	tinydir_file file;
	if (tinydir_open(&dir, input_dir) != 0) {
		printf("Error opening directory %s\n", input_dir);
		return EXIT_FAILURE;
	}

	assert(dir.has_next > 0);

	printf("Archiving map directory %s\n", input_dir);

	while (dir.has_next) {
		tinydir_readfile(&dir, &file);
		if (strncasecmp(file.extension, "bsp", 3) == 0) {
			archive_bsp(file.path, output_path, gamedir);
		}
		tinydir_next(&dir);
	}
	tinydir_close(&dir);

	return EXIT_SUCCESS;
}

int archive_bsp(const char* bsp_path, const char* output_path, const char* gamedir) {
	//TODO: enum on exit statuses
	int rc = EXIT_SUCCESS;
	char archivename[MAX_PATH];

	if(!get_bsp_name(bsp_path, bspname)) {
		printf("Error getting bsp name from path %s", bsp_path);
		rc = EXIT_FAILURE;
		goto exit;
	}
	if(g_verbose) {
		printf("Processing map: %s\n", bsp_path);
	}
	else {
		printf("Processing map: %s.bsp\n", bspname);
	}

	if(bsp_get_deps(bsp_path, bspname)) {
		rc = EXIT_FAILURE;
		goto exit;
	}
	
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
		
		if(!g_noexclude && hashtable_contains(exclude_table, dep_name)) {
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
		printf("Archived map '%s' successfully: %u files added, %u skipped, %u could not be found.\n", bspname, dep_success, dep_skipped, dep_missing);
	}
	else {
		printf("Failed archiving map '%s'\n", bspname);
		remove(archivename);
		rc = EXIT_FAILURE;
	}	
exit:
	free_dependency_list();
	return rc;
}
