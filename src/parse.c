#include <parse.h>
#include <string.h>

#include <config.h>

static int expandfile(struct string *expanded, char *filename, int level);
static struct string *getstring(FILE *file, char end);

int parsefile(char *template, FILE *out) {
	struct string *expanded;
	expanded = newstring();
	if (expanded == NULL)
		return 1;
	if (expandfile(expanded, template, 0))
		return 1;
	fwrite(expanded->data, 1, expanded->len, stdout);
	return 0;
}

static int expandfile(struct string *expanded, char *filename, int level) {
	FILE *file;
	int c, linenum;
	file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file %s\n", filename);
		return 1;
	}
	linenum = 1;
	if (level >= MAX_INCLUDE_DEPTH) {
		fprintf(stderr, "The include depth has reached %d, quitting\n",
				level);
		fclose(file);
		return 1;
	}
	for (;;) {
		c = fgetc(file);
		switch (c) {
		case ESCAPE_CHAR:
			c = fgetc(file);
			switch (c) {
			case ESCAPE_CHAR:
				if (appendchar(expanded, ESCAPE_CHAR))
					goto error;
				break;
			case VAR_CHAR:
				if (appendchar(expanded, ESCAPE_CHAR))
					goto error;
				for (;;) {
					if (c == EOF)
						goto error;
					if (appendchar(expanded, c))
						goto error;
					if (c == ESCAPE_CHAR)
						break;
					c = fgetc(file);
				}
				break;
			case INCLUDE_CHAR: {
				struct string *inclname;
				inclname = getstring(file, ESCAPE_CHAR);
				if (inclname == NULL)
					goto error;
				if (expandfile(expanded, inclname->data,
							level + 1))
					return 1;
				freestring(inclname);
				break;
			}
			default:
				fprintf(stderr, "Line %d: Invalid escape\n",
						linenum);
				goto error;
			}
			break;
		case '\n':
			++linenum;
			goto casedefault;
		case EOF:
			goto end;
		default: casedefault:
			if (appendchar(expanded, c))
				goto error;
			break;
		}
	}
end:
	fclose(file);
	return 0;
error:
	fclose(file);
	return 1;
}

static struct string *getstring(FILE *file, char end) {
	struct string *ret;
	int c;
	ret = newstring();
	if (ret == NULL)
		return NULL;
	for (;;) {
		c = fgetc(file);
		if (c == EOF)
			goto error;
		if (c == end) {
			if (appendchar(ret, '\0'))
				goto error;
			return ret;
		}
		if (appendchar(ret, c))
			goto error;
	}
error:
	freestring(ret);
	return NULL;
}
