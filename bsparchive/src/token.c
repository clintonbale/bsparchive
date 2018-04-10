#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "token.h"

const char* stream;
EntityToken token;

void next_token(void) {
repeat:
	switch (*stream) {
	case ' ': case '\n': case '\r': case '\t': case '\v':
		while (isspace(*stream)) {
			stream++;
		}
		goto repeat;
		break;
	case '{':
		token.type = TOKEN_BEGIN_ENT;
		stream++;
		break;
	case '}':
		token.type = TOKEN_END_ENT;
		stream++;
		break;
	case '"': {
		const char* start = ++stream;
		while (*stream != '"') {
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
		//fatal("Unexpected token type: %c", *stream);
		stream++;
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