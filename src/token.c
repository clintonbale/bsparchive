#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "token.h"

const char* stream;
EntityToken token;

bool string_token_end() {
	const char* s = stream;

	if (*s != '"')
		return FALSE;

	// spaces after quote is end of value
	if (isspace(*(s + 1)))
		return TRUE;

	// comments immediately after quote is end only if no quote before newline
	if (*(s + 1) == '/' && *(s + 2) == '/') {
		while (*s != '\n' && *s != '\r') {
			if (*s++ == '"')
				return TRUE;
		}
	}

	return FALSE;
}

void next_token(void) {
repeat:
	switch (*stream) {
	case '/':
		assert(*(stream + 1) == '/');

		while (*stream != '\n' && *stream != '\r') {
			stream++;
		}
		goto repeat;
		break;
	case ' ': case '\n': case '\r': case '\t': case '\v':
		while (isspace(*stream)) {
			stream++;
			if (*stream & 0x80) break;
		}
		goto repeat;
		break;
	case '{': case '(':
		token.type = TOKEN_BEGIN_ENT;
		stream++;
		break;
	case '}': case ')':
		token.type = TOKEN_END_ENT;
		stream++;
		break;
	case '"': {
		const char* start = ++stream;
		while (!string_token_end()) {
			stream++;
		}
		token.start = start;
		token.end = stream++;
		token.type = TOKEN_STR;
		break;
	}
	case EOF:
	case 0:
		token.type = TOKEN_NULL;
		break;
	default:
		stream++;
		goto repeat;
		break;
	}
}

void token_test(void) {
	char* s = "{\n\"origin\" \"490 562 -44\"\n}";
	stream = s;

	char key[32];
	char value[32];

	next_token();
	assert(match_token(TOKEN_BEGIN_ENT));

	assert(is_token(TOKEN_STR));
	strncpy(key, token.start, token.end - token.start);
	key[token.end - token.start] = 0;
	assert(strcmp(key, "origin") == 0);

	assert(match_token(TOKEN_STR));
	strncpy(value, token.start, token.end - token.start);
	value[token.end - token.start] = 0;
	assert(strcmp(value, "490 562 -44") == 0);

	next_token();
	assert(match_token(TOKEN_END_ENT));
}