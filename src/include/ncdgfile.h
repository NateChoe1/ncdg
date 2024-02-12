#ifndef HAVE_NCDGFILE
#define HAVE_NCDGFILE

#include <stdio.h>

struct ncdgfile {
	int (*putc)(struct ncdgfile *file, int c);
	int (*puts)(struct ncdgfile *file, char *s);
	void (*free)(struct ncdgfile *file);
	void *handle;
};

struct ncdgfile *file2ncdg(FILE *file);
struct ncdgfile *stringfile(void);

#endif
