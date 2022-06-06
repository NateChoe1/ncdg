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

static int expandfile(struct expandfile *ret, char *filename, int level);
static struct string *getstring(FILE *file, char end);

int parsefile(char *template, FILE *out) {
	struct expandfile *expanded;
	expanded = malloc(sizeof *expanded);
	if (expanded == NULL)
		goto error1;
	expanded->data = newstring();
	if (expanded->data == NULL)
		goto error2;
	expanded->vars = newvector(struct var);
	if (expanded->vars == NULL)
		goto error3;
	if (expandfile(expanded, template, 0))
		goto error4;
	{
		long i;
		for (i = 0; i < expanded->data->len; ++i) {
			if (expanded->data->data[i] == ESCAPE_CHAR) {
				switch (expanded->data->data[++i]) {
				case ESCAPE_CHAR:
					fputc(ESCAPE_CHAR, out);
					break;
				case VAR_CHAR: {
					long start;
					int j;
					struct string *data;
					struct vector *vars;
					char *varname;
					data = expanded->data;
					start = ++i;
					while (data->data[i] != ESCAPE_CHAR &&
							i < data->len)
						++i;
					data->data[i] = '\0';
					varname = data->data + start;
					vars = expanded->vars;
					for (j = 0; j < vars->len; ++j) {
						struct var var;
						var = getvector(vars,
								struct var, j);
						if (strcmp(var.var->data,
								varname) == 0) {
							fputs(var.value->data,
									out);
						}
					}
				}
				}
			}
			else
				fputc(expanded->data->data[i], out);
		}
	}
	return 0;
error4:
	{
		int i;
		for (i = 0; i < expanded->vars->len; ++i) {
			struct var var;
			var = getvector(expanded->vars, struct var, i);
			freestring(var.var);
			freestring(var.value);
		}
	}
	freevector(expanded->vars);
error3:
	freestring(expanded->data);
error2:
	free(expanded);
error1:
	return 1;
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
			case VAR_CHAR:
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
