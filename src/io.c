#include <stdio.h>
#include <stdlib.h>

#include <io.h>

static char *append(char *str, char c, size_t *len, size_t *alloc);

void ungetline(struct linefile *file, char *line) {
	file->prevline = line;
}

char *getline(struct linefile *file) {
	size_t alloc;
	size_t len;
	char *ret;
	int c;

	if (file->prevline != NULL) {
		ret = file->prevline;
		file->prevline = NULL;
		return ret;
	}

	c = fgetc(file->file);
	if (c == EOF)
		return NULL;
	ungetc(c, file->file);

	alloc = 80;
	len = 0;
	ret = malloc(alloc);
	if (ret == NULL)
		return NULL;

	for (;;) {
		c = fgetc(file->file);
		if (c == '\n') {
			ret[len] = '\0';
			return ret;
		}
		if (c == '\t') {
			while (len % 4 != 0) {
				ret = append(ret, ' ', &len, &alloc);
				if (ret == NULL)
					return NULL;
			}
		}
		else {
			ret = append(ret, c, &len, &alloc);
			if (ret == NULL)
				return NULL;
		}
	}
}

struct linefile *newlinefile(FILE *file) {
	struct linefile *ret;
	ret = malloc(sizeof *ret);
	ret->prevline = NULL;
	ret->file = file;
	return ret;
}

void freelinefile(struct linefile *file) {
	free(file->prevline);
	fclose(file->file);
	free(file);
}

static char *append(char *str, char c, size_t *len, size_t *alloc) {
	if (*len >= *alloc) {
		char *newstr;
		*alloc *= 2;
		newstr = realloc(str, *alloc);
		if (newstr == NULL) {
			free(str);
			return NULL;
		}
		str = newstr;
	}
	str[(*len)++] = c;
	return str;
}
