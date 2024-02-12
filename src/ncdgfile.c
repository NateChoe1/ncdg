#include <stdio.h>
#include <stdlib.h>

#include <strings.h>
#include <ncdgfile.h>

static int fileputc(struct ncdgfile *file, int c);
static int fileputs(struct ncdgfile *file, char *s);
static void filefree(struct ncdgfile *file);

static int stringputc(struct ncdgfile *file, int c);
static int stringputs(struct ncdgfile *file, char *s);
static void stringfree(struct ncdgfile *file);

struct ncdgfile *file2ncdg(FILE *file) {
	struct ncdgfile *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}
	ret->putc = fileputc;
	ret->puts = fileputs;
	ret->free = filefree;
	ret->handle = (void *) file;
	return ret;
}

struct ncdgfile *stringfile(void) {
	struct ncdgfile *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		goto error1;
	}
	if ((ret->handle = (void *) newstring()) == NULL) {
		goto error2;
	}
	ret->putc = stringputc;
	ret->puts = stringputs;
	ret->free = stringfree;
	return ret;
error2:
	free(ret);
error1:
	return NULL;
}

static int fileputc(struct ncdgfile *file, int c) {
	return fputc(c, (FILE *) file->handle) == EOF ? -1 : 0;
}

static int fileputs(struct ncdgfile *file, char *s) {
	return fputs(s, (FILE *) file->handle) == EOF ? -1 : 0;
}

static void filefree(struct ncdgfile *file) {
	free(file);
}

static int stringputc(struct ncdgfile *file, int c) {
	return appendchar((struct string *) file->handle, c) ? -1 : 0;
}

static int stringputs(struct ncdgfile *file, char *s) {
	long i;
	for (i = 0; s[i] != '\0'; ++i) {
		if (appendchar((struct string *) file->handle, s[i])) {
			return -1;
		}
	}
	return 0;
}

static void stringfree(struct ncdgfile *file) {
	freestring((struct string *) file->handle);
	free(file);
}
