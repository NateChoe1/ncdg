/*
   ncdg - A program to help generate natechoe.dev
   Copyright (C) 2022  Nate Choe (natechoe9@gmail.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <template.h>

struct linefile {
	char *prevline;
	FILE *file;
};

enum paratype {
	NORMAL,
	EMPTY,
	H1, H2, H3, H4, H5, H6,
	BLOCKQUOTE,
	CODESPACE, CODEBACK,
	UL, OL,
	HL
};

enum inlinetype {
	ITALIC,
	BOLD,
	CODE
};

static const struct {
	char c;
	char *escape;
} escapes[] = {
	{'&', "&amp;"},
	{';', "&semi;"},
	{'<', "&lt;"},
	{'>', "&gt;"},
};

static int parsepara(struct linefile *infile, FILE *outfile);
static enum paratype identifypara(char *line, char **contentret);

static char *untrail(char *line);
static size_t reallen(char *line);
static int islinebreak(char *line);

static int paraeasycase(struct linefile *infile, FILE *outfile,
		char *line, char *buff,
		char *tag, enum paratype type);
static int parahardcase(struct linefile *infile, FILE *outfile,
		char *line, char *buff,
		char *vars, char *linetag, char *tag, enum paratype type);
static int paracodecase(struct linefile *infile, FILE *outfile,
		char *line, char *buff,
		char *vars, enum paratype type);
static long strsearch(char *data, long start, size_t datalen, char c, int reps);
/* strsearch finds instances in data with reps repetitions of c. returns the
 * last instance in the first group. For example:
 *
 * c = '.', reps = 2, data = " ...", returns 2
 * c = '.', reps = 2, data = ".. ...", returns 4
 * c = '.', reps = 1, data = " ...", returns 3
 * */

static long writelinked(char *data, long i, size_t len, char *tag,
		FILE *outfile);

static int writeescape(char c, FILE *outfile);
static int writedata(char *data, size_t len, FILE *outfile);
static int writesimple(char *data, size_t len, FILE *outfile);

static void ungetline(struct linefile *file, char *line);
static char *getline(struct linefile *file);

static const char *escapedchars = "!\"#%&'()*,./:;?@[\\]^{|}~";

int parsetemplate(FILE *infile, FILE *outfile) {
	struct linefile realin;
	realin.prevline = NULL;
	realin.file = infile;
	while (parsepara(&realin, outfile) == 0) ;

	return 0;
}

static int parsepara(struct linefile *infile, FILE *outfile) {
	for (;;) {
		char *line, *buff;
		/* line exists for the explicit purpose of being freed later */
		enum paratype type;

		line = getline(infile);
		if (line == NULL)
			return 1;
		type = identifypara(line, &buff);

		buff = untrail(buff);

		if (buff[0] == '\0') {
			free(line);
			continue;
		}

		switch (type) {
#define EASY_CASE(enumtype, tag) \
		case enumtype: \
			paraeasycase(infile, outfile, line, buff, \
					tag, enumtype); \
			return 0;
#define HARD_CASE(enumtype, tag, linetag, vars) \
		case enumtype: \
			parahardcase(infile, outfile, line, buff, \
					vars, linetag, tag, enumtype); \
			return 0;
#define CODE_CASE(enumtype, vars) \
		case enumtype: \
			paracodecase(infile, outfile, line, buff, \
					vars, enumtype); \
			return 0;
		EASY_CASE(H1, "h1");
		EASY_CASE(H2, "h2");
		EASY_CASE(H3, "h3");
		EASY_CASE(H4, "h4");
		EASY_CASE(H5, "h5");
		EASY_CASE(H6, "h6");
		HARD_CASE(NORMAL, "p", NULL, NULL);
		HARD_CASE(BLOCKQUOTE, "blockquote", NULL, NULL);
		HARD_CASE(UL, "ul", "li", NULL);
		HARD_CASE(OL, "ol", "li", NULL);
		CODE_CASE(CODESPACE, "class='block'");
		CODE_CASE(CODEBACK, "class='block'");
		case HL:
			fputs("<hr />", outfile);
			free(line);
			return 0;
		case EMPTY:
			free(line);
			continue;
		}
	}
}

static int isbreak(char *line) {
	int count, i;
	char whitechar;
	count = 0;
	whitechar = '\0';
	for (i = 0; line[i] != '\0'; ++i) {
		if (line[i] == line[0])
			++count;
		else if (line[i] == ' ' || line[i] == '\t') {
			if (whitechar == '\0')
				whitechar = line[i];
			if (whitechar != line[i])
				return 0;
		}
		else
			return 0;
	}
	return count >= 3;
	return 0;
}

static enum paratype identifypara(char *line, char **contentret) {
	int i;
	for (i = 0; i < 4; ++i) {
		if (line[i] == ' ')
			continue;
		if (line[i] == '\0')
			return EMPTY;
		goto whitegone;
	}

	*contentret = line + i;
	return CODESPACE;

whitegone:
	line += i;
	/* At this point, line has no extraneous trailing whitespace */
	switch (line[0]) {
	case '\0':
		return EMPTY;
	case '#':
		for (i = 0; i < 6 && line[i] == '#'; ++i) ;
		*contentret = line + i;
		if (line[i] != '\0' && line[i] != ' ')
			goto normal;
		return H1 + i - 1;
	case '>':
		*contentret = line + 1;
		return BLOCKQUOTE;
	case '*':
		if (isbreak(line))
			return HL;
		*contentret = line + 1;
		return UL;
	case '-': case '_':
		if (isbreak(line))
			return HL;
		goto normal;
	case '`':
		for (i = 0; i < 3; ++i)
			if (line[i] != '`')
				goto normal;
		return CODEBACK;
	default:
		if (isdigit(line[0])) {
			for (i = 0; isdigit(line[i]); ++i) ;
			if (line[i] == '.' || line[i] == ')') {
				*contentret = line + i + 1;
				return OL;
			}
		}
		goto normal;
	normal:
		*contentret = line;
		return NORMAL;
	}
}

static char *untrail(char *line) {
	while (isspace(line[0]))
		++line;
	return line;
}

static size_t reallen(char *line) {
	size_t fakelen;
	fakelen = strlen(line);
	if (line[fakelen - 1] == '\\')
		--fakelen;
	while (isspace(line[fakelen]))
		--fakelen;
	return fakelen;
}

static int islinebreak(char *line) {
	size_t len;
	int i;
	len = strlen(line);
	if (line[len - 1] == '\\')
		return 1;
	if (len < 2)
		return 0;
	for (i = 0; i < 2; ++i)
		if (!isspace(line[len - i - 1]))
			return 0;
	return 1;
}

static int paraeasycase(struct linefile *infile, FILE *outfile,
		char *line, char *buff,
		char *tag, enum paratype type) {
	size_t writelen;

	writelen = reallen(buff);

	fprintf(outfile, "<%s>", tag);
	for (;;) {
		writedata(buff, writelen, outfile);
		free(line);
		line = getline(infile);
		if (line == NULL)
			break;
		if (identifypara(line, &buff) != type) {
			ungetline(infile, line);
			line = NULL;
			break;
		}
		else
			buff = untrail(buff);
	}
	fprintf(outfile, "</%s>", tag);

	free(line);
	return 0;
}

static int parahardcase(struct linefile *infile, FILE *outfile,
		char *line, char *buff,
		char *vars, char *linetag, char *tag, enum paratype type) {
	size_t writelen;

	if (vars == NULL)
		fprintf(outfile, "<%s>", tag);
	else
		fprintf(outfile, "<%s %s>", tag, vars);
	for (;;) {
		writelen = reallen(buff);

		if (linetag != NULL)
			fprintf(outfile, "<%s>", linetag);
		writedata(buff, writelen, outfile);
		if (islinebreak(line))
			fputs("<br />", outfile);
		if (linetag != NULL)
			fprintf(outfile, "</%s>", linetag);

		free(line);
		line = getline(infile);
		if (line == NULL)
			break;
		if (identifypara(line, &buff) != type) {
			buff = untrail(line);
			if (buff[0] == '\0') {
				free(line);
				line = NULL;
				break;
			}
		}
		else
			buff = untrail(buff);
		fputc(' ', outfile);
	}
	fprintf(outfile, "</%s>", tag);

	free(line);
	return 0;
}

static int paracodecase(struct linefile *infile, FILE *outfile,
		char *line, char *buff,
		char *vars, enum paratype type) {
	int seenfirst;
	enum paratype newtype;

	if (type != CODESPACE && type != CODEBACK)
		return 1;

	if (vars == NULL)
		fputs("<code>", outfile);
	else
		fprintf(outfile, "<code %s>", vars);
	seenfirst = 0;
	newtype = type;
	for (;;) {
		if ((type == CODEBACK && type != newtype) ||
		     newtype == CODESPACE) {
			if (seenfirst)
				fputs("<br />", outfile);
			seenfirst = 1;
		}

		if (newtype != CODEBACK)
			writesimple(buff, -1, outfile);

		free(line);
		line = getline(infile);
		if (line == NULL)
			return 1;

		newtype = identifypara(line, &buff);
		if (type == CODEBACK && newtype == CODEBACK)
			break;
		if (type == CODESPACE && newtype != type) {
			ungetline(infile, line);
			break;
		}
	}
	fputs("</code>", outfile);

	if (type == CODEBACK)
		free(line);
	return 0;
}

static long strsearch(char *data, long start, size_t datalen,
		char c, int reps) {
	long i;

	for (i = start; data[i] == c; ++i) ;

	while (i + reps - 1 < datalen) {
		int j;
		for (j = 0; j < reps; ++j)
			if (data[i + j] != c)
				goto failure;
		goto success;
		continue;
failure:
		++i;
	}
	return -1;

success:
	while (data[i + reps] == c && i + reps < datalen)
		++i;
	return i;
}

static long writelinked(char *data, long i, size_t len, char *tag,
		FILE *outfile) {
	long linkend, textend;
	textend = strsearch(data, i, len, ']', 1);
	if (textend < 0)
		return -1;
	linkend = strsearch(data, textend, len, ')', 1);
	if (linkend < 0)
		return -1;
	if (strcmp(tag, "a") == 0) {
		fputs("<a href='", outfile);
		writesimple(data + textend + 2,
				linkend - textend - 2, outfile);
		fputs("'>", outfile);
		writesimple(data + i + 1,
				textend - i - 1, outfile);
		fputs("</a>", outfile);
		return linkend;
	}
	else if (strcmp(tag, "img") == 0) {
		fputs("<img src='", outfile);
		writesimple(data + textend + 2,
				linkend - textend - 2, outfile);
		fputs("' alt='", outfile);
		writesimple(data + i + 1,
				textend - i - 1, outfile);
		fputs("'>", outfile);
		return linkend;
	}
	return -1;
}

static int writeescape(char c, FILE *outfile) {
	int i;
	for (i = 0; i < sizeof escapes / sizeof *escapes; ++i) {
		if (escapes[i].c == c) {
			fputs(escapes[i].escape, outfile);
			return 0;
		}
	}
	fputc(c, outfile);
	return 0;
}

static int writedata(char *data, size_t len, FILE *outfile) {
	long i;
	long start;
	long end;
	for (i = 0; i < len; ++i) {
		switch (data[i]) {
#define STANDOUT_CHAR(c) \
		case c: \
			if (data[i + 1] == c) { \
				start = i + 2; \
				end = strsearch(data, start, len, \
						c, 2); \
				goto bold; \
			} \
			start = i + 1; \
			end = strsearch(data, start, len, c, 1); \
			goto italic;
		STANDOUT_CHAR('*');
		STANDOUT_CHAR('_');
		italic:
			if (end < 0)
				goto normal;
			fputs("<i>", outfile);
			writedata(data + start, end - start, outfile);
			fputs("</i>", outfile);
			i = end;
			break;
		bold:
			if (end < 0)
				goto normal;
			fputs("<b>", outfile);
			writedata(data + start, end - start, outfile);
			fputs("</b>", outfile);
			i = end + 1;
			break;

		case '`':
			end = strsearch(data, i, len, '`', 1);
			if (end < 0)
				goto normal;
			fputs("<code>", outfile);
			writedata(data + i, end - i, outfile);
			fputs("</code>", outfile);
			i = end;
			break;
		case '[':
			end = writelinked(data, i, len, "a", outfile);
			if (end < 0)
				goto normal;
			i = end;
			break;
		case '!':
			end = writelinked(data, i + 1, len, "img", outfile);
			if (end < 0)
				goto normal;
			i = end;
			break;
		case '\\':
			if (i == len ||
				strchr(escapedchars, data[i+1]) == NULL) {
				fputc('\\', outfile);
				break;
			}
			++i;
			goto normal;
		default: normal:
			writeescape(data[i], outfile);
			break;
		}
	}
	return 0;
}

static int writesimple(char *data, size_t len, FILE *outfile) {
	long i;
	for (i = 0; (len < 0 && data[i] != '\0') || i < len; ++i) {
		if (data[i] == '\\')
			if (strchr(escapedchars, data[i]) == NULL)
				fputc('\\', outfile);
		writeescape(data[i], outfile);
	}
	return 0;
}

static void ungetline(struct linefile *file, char *line) {
	file->prevline = line;
}

static char *getline(struct linefile *file) {
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
		if (len >= alloc) {
			char *newret;
			alloc *= 2;
			newret = realloc(ret, alloc);
			if (newret == NULL) {
				free(ret);
				return NULL;
			}
			ret = newret;
		}
		c = fgetc(file->file);
		if (c == '\n') {
			ret[len] = '\0';
			return ret;
		}
		ret[len++] = c;
	}
}
