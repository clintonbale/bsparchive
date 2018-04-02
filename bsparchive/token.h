#pragma once
#include <stdbool.h>
#include "common.h"

typedef enum TokenType {
	TOKEN_BEGIN_ENT,
	TOKEN_END_ENT,
	TOKEN_STR,
	TOKEN_NULL
} TokenType;

typedef struct Token {
	TokenType type;
	const char* start;
	const char* end;
} Token;

const char* stream;
Token token;

void next_token();

inline bool is_token(TokenType type) {
	return token.type == type;
}

inline bool match_token(TokenType type) {
	if (is_token(type)) {
		next_token();
		return true;
	}
	return false;
}

inline bool expect_token(TokenType type) {
	if (is_token(type)) {
		next_token();
		return true;
	}
	else {
		fatal("Expected token");
		return false;
	}
}

void token_test();