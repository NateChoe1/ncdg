#include <stdio.h>
#include <stdlib.h>

#include <strings.h>

struct string *newstring(void) {
	struct string *ret;
	ret = malloc(sizeof *ret);
	if (ret == NULL)
		goto error1;
	ret->len = 0;
	ret->alloc = 200;
	ret->data = malloc(ret->alloc);
	if (ret->data == NULL)
		goto error2;
	return ret;
error2:
	free(ret);
error1:
	return NULL;
}

struct string *charp2s(char *s) {
	struct string *ret;
	long i;
	if ((ret = newstring()) == NULL) {
		goto error1;
	}
	for (i = 0; s[i] != '\0'; ++i) {
		if (appendchar(ret, s[i])) {
			goto error2;
		}
	}
	if (appendchar(ret, '\0')) {
		goto error2;
	}
	return ret;
error2:
	freestring(ret);
error1:
	return NULL;
}

int appendchar(struct string *string, char c) {
	if (string->len+1 >= string->alloc) {
		char *newdata;
		size_t newalloc;
		newalloc = string->alloc * 2;
		newdata = realloc(string->data, newalloc);
		if (newdata == NULL)
			return 1;
		string->data = newdata;
		string->alloc = newalloc;
	}
	string->data[string->len++] = c;
	string->data[string->len] = 0;
	return 0;
}

void freestring(struct string *string) {
	free(string->data);
	free(string);
}

void resetstring(struct string *string) {
	string->len = 0;
}
