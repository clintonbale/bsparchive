#pragma once
#include <stdbool.h>
#include "common.h"

typedef enum EntityTokenType {
	TOKEN_BEGIN_ENT,
	TOKEN_END_ENT,
	TOKEN_STR,
	TOKEN_NULL
} EntityTokenType;

typedef struct Token {
	EntityTokenType type;
	const char* start;
	const char* end;
} EntityToken;

const char* stream;
EntityToken token;

void next_token(void);

inline bool is_token(EntityTokenType type) {
	return token.type == type;
}

inline bool match_token(EntityTokenType type) {
	if (is_token(type)) {
		next_token();
		return true;
	}
	return false;
}

inline bool expect_token(EntityTokenType type) {
	if (is_token(type)) {
		next_token();
		return true;
	}
	else {
		printf("Error: failed parsing entity token near:\n\n%.200s\n\n", stream-100);
		return false;
	}
}

void token_test(void);