#ifndef HAVE_STRING
#define HAVE_STRING

#include <stddef.h>

struct string {
	size_t len;
	size_t alloc;
	char *data;
};

struct string *newstring();
int appendchar(struct string *string, char c);
void freestring(struct string *string);
void resetstring(struct string *string);

#endif
