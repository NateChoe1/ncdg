#include <config.h>

#ifdef ALLOW_SHELL
#define _POSIX_C_SOURCE 2
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <parse.h>
#include <vector.h>
#include <strings.h>

struct var {
	struct string *var;
	struct string *value;
};

struct expandfile {
	struct string *data;
	struct vector *vars;
	/* This is a vector of struct var */
};

struct minstate {
	int ignore;
	/* If this is set, then ignore the current whitespace group */
	int isspace;
	/* If this is set, then we've seen whitespace on the previous char. */
};

static int expandfile(struct expandfile *ret, char *filename, int level);
static int writefile(struct expandfile *file, FILE *out);
static struct string *getstring(FILE *file, char end);
static void initminstate(struct minstate *state);
static void mputs(struct minstate *state, char *s, FILE *file);
static void mputc(struct minstate *state, char c, FILE *file);
static long putvar(long i, struct minstate *s, FILE *out,
		const struct string *file, const struct vector *vars);
static int defvars(struct expandfile *expanded, char *filename);

int parsefile(char *template, FILE *out) {
	struct expandfile expanded;
	int ret;
	expanded.data = newstring();
	if (expanded.data == NULL) {
		ret = 1;
		goto error1;
	}
	expanded.vars = newvector(struct var);
	if (expanded.vars == NULL) {
		ret = 1;
		goto error2;
	}
	if (defvars(&expanded, template)) {
		ret = 1;
		goto error3;
	}
	if (expandfile(&expanded, template, 0)) {
		ret = 1;
		goto error3;
	}
	ret = writefile(&expanded, out);
error3:
	{
		int i;
		for (i = 0; i < expanded.vars->len; ++i) {
			struct var var;
			var = getvector(expanded.vars, struct var, i);
			freestring(var.var);
			freestring(var.value);
		}
	}
	freevector(expanded.vars);
error2:
	freestring(expanded.data);
error1:
	return ret;
}

static int writefile(struct expandfile *file, FILE *out) {
	long i;
	struct minstate s;
	initminstate(&s);
	for (i = 0; i < file->data->len; ++i) {
		if (file->data->data[i] == ESCAPE_CHAR) {
			const struct string *data = file->data;
			const struct vector *vars = file->vars;
			switch (data->data[++i]) {
			case ESCAPE_CHAR:
				mputc(&s, ESCAPE_CHAR, out);
				break;
			case VAR_CHAR:
				i = putvar(i, &s, out, data, vars);
				if (i < 0)
					return 1;
				break;
			case AUTOESCAPE_CHAR:
				if (data->data[++i] != '\n')
					--i;
				for (++i; i < data->len; ++i) {
					switch (data->data[i]) {
					case '&':
						fputs("&amp;", out);
						break;
					case ';':
						fputs("&semi;", out);
						break;
					case '<':
						fputs("&lt;", out);
						break;
					case '>':
						fputs("&gt;", out);
						break;
					case ESCAPE_CHAR:
						if (data->data[i + 1] != ESCAPE_CHAR)
							goto autoescapeend;
						++i;
						/* fallthrough */
					default:
						fputc(data->data[i], out);
						break;
					}
				}
autoescapeend:
				break;
			case NOMINIFY_CHAR:
				if (data->data[++i] != '\n')
					--i;
				for (++i; i < data->len; ++i) {
					if (data->data[i] == ESCAPE_CHAR) {
						if (data->data[i + 1] != ESCAPE_CHAR)
							break;
						++i;
					}
					fputc(data->data[i], out);
				}
				break;
#ifdef ALLOW_SHELL
			case SHELL_CHAR: {
				FILE *process;
				long start;
				start = ++i;
				while (data->data[i] != ESCAPE_CHAR)
					++i;
				data->data[i] = '\0';
				process = popen(data->data + start, "r");
				for (;;) {
					int c;
					c = fgetc(process);
					if (c == EOF)
						break;
					mputc(&s, c, out);
				}
				pclose(process);
				break;
			}
#endif
			default:
				fprintf(stderr, "Error in expansion phase: Unknown escape %c%c\n", ESCAPE_CHAR, data->data[i]);
				return 1;
			}
		}
		else
			mputc(&s, file->data->data[i], out);
	}
	return 0;
}

static int expandfile(struct expandfile *ret, char *filename, int level) {
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
				if (appendchar(ret->data, ESCAPE_CHAR))
					goto error;
				if (appendchar(ret->data, ESCAPE_CHAR))
					goto error;
				break;
			case VAR_CHAR: case AUTOESCAPE_CHAR: case NOMINIFY_CHAR:
			case SHELL_CHAR:
				if (appendchar(ret->data, ESCAPE_CHAR))
					goto error;
				for (;;) {
					if (c == EOF)
						goto error;
					if (appendchar(ret->data, c))
						goto error;
					if (c == '\n')
						++linenum;
					if (c == ESCAPE_CHAR) {
						c = fgetc(file);
						if (c == ESCAPE_CHAR) {
							if (appendchar(
								ret->data, c))
								goto error;
						}
						else {
							ungetc(c, file);
							break;
						}
					}
					c = fgetc(file);
				}
				break;
			case SET_CHAR: {
				struct var var;
				var.var = getstring(file, ' ');
				var.value = getstring(file, ESCAPE_CHAR);
				addvector(ret->vars, &var);
				break;
			}
			case INCLUDE_CHAR: {
				struct string *inclname;
				inclname = getstring(file, ESCAPE_CHAR);
				if (inclname == NULL)
					goto error;
				if (expandfile(ret, inclname->data,
							level + 1))
					return 1;
				freestring(inclname);
				break;
			}
			default:
				fprintf(stderr, "Line %d: Invalid escape %c\n",
						linenum, c);
				goto error;
			}
			break;
		case '\n':
			++linenum;
			goto casedefault;
		case EOF:
			goto end;
		default: casedefault:
			if (appendchar(ret->data, c))
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

static void initminstate(struct minstate *state) {
	memset(state, 0, sizeof *state);
}

static void mputs(struct minstate *state, char *s, FILE *file) {
	int i;
	for (i = 0; s[i] != '\0'; ++i)
		mputc(state, s[i], file);
}

static void mputc(struct minstate *state, char c, FILE *file) {
	if (isspace(c))
		state->isspace = 1;
	else {
		if (!state->ignore && state->isspace && c != '>')
			fputc(' ', file);
		fputc(c, file);
		state->ignore = c == '<';
		state->isspace = 0;
	}
}

static long putvar(long i, struct minstate *s, FILE *out,
		const struct string *file, const struct vector *vars) {
	long start;
	for (;;) {
		int j;
		start = ++i;
		while (file->data[i] != SEPARATOR_CHAR &&
		       file->data[i] != ESCAPE_CHAR &&
		       i < file->len)
			++i;
		if (i >= file->len)
			return -1;
		for (j = 0; j < vars->len; ++j) {
			const struct var var = getvector(vars, struct var, j);
			if (var.var->len - 1 != i - start)
				continue;
			/* -1 because var.var->len includes the \0. */
			if (memcmp(var.var->data, file->data + start,
					i - start) == 0) {
				mputs(s, var.value->data, out);
				goto end;
			}
		}
		if (file->data[i] != SEPARATOR_CHAR)
			goto end;
	}
end:
	while (file->data[i] != ESCAPE_CHAR && i < file->len)
		++i;
	if (i == file->len)
		return -1;
	return i;
}

static int defvars(struct expandfile *expanded, char *filename) {
	struct var scratch;
	if ((scratch.var = charp2s("_filename")) == NULL) {
		goto error1;
	}
	if ((scratch.value = charp2s(filename)) == NULL) {
		goto error2;
	}
	if (addvector(expanded->vars, &scratch)) {
		goto error3;
	}
	return 0;
error3:
	freestring(scratch.value);
error2:
	freestring(scratch.var);
error1:
	return 1;
}
