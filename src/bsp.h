#pragma once
#include <stdint.h>
#include <stdbool.h>

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

typedef void(*bsp_entity_reader)(char*, char*);

char* bsp_open_entities(const char* path);
bool bsp_read_entities(bsp_entity_reader reader);