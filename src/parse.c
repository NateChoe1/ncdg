#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <parse.h>
#include <vector.h>
#include <strings.h>

#include <config.h>

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
			case VAR_CHAR: {
				long start;
				int j;
				char *varname;
				start = ++i;
				while (data->data[i] != ESCAPE_CHAR &&
						i < data->len)
					++i;
				data->data[i] = '\0';
				varname = data->data + start;
				vars = file->vars;
				for (j = 0; j < vars->len; ++j) {
					struct var var;
					var = getvector(vars,
							struct var, j);
					if (strcmp(var.var->data,
							varname) == 0) {
						mputs(&s, var.value->data,
								out);
					}
				}
				break;
			}
			case AUTOESCAPE_CHAR:
				for (++i; data->data[i] != ESCAPE_CHAR &&
						i < data->len; ++i) {
					switch (data->data[i]) {
					case '&':
						fputs("&amp;", out);
						break;
					case ';':
						fputs("&semi;", out);
					case '<':
						fputs("&lt;", out);
						break;
					case '>':
						fputs("&gt;", out);
						break;
					default:
						fputc(data->data[i], out);
						break;
					}
				}
				break;
			case NOMINIFY_CHAR:
				for (++i; data->data[i] != ESCAPE_CHAR &&
						i < data->len; ++i)
					fputc(data->data[i], out);
				break;
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
				if (appendchar(ret->data, ESCAPE_CHAR))
					goto error;
				for (;;) {
					if (c == EOF)
						goto error;
					if (appendchar(ret->data, c))
						goto error;
					if (c == ESCAPE_CHAR)
						break;
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
		if (!state->ignore && state->isspace && c != '<' && c != '>')
			fputc(' ', file);
		fputc(c, file);
		state->ignore = c == '<' || c == '>';
		state->isspace = 0;
	}
}
