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

static void ungetline(struct linefile *file, char *line);
static char *getline(struct linefile *file);

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

static enum paratype identifypara(char *line, char **contentret) {
	int i, count;
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
		return H1 + i - 1;
	case '>':
		*contentret = line + 1;
		return BLOCKQUOTE;
	case '-':
		count = 0;
		for (i = 0; line[i] != '\0'; ++i) {
			if (line[i] == '-')
				++count;
			if (count == 3)
				return HL;
		}
	case '*':
		*contentret = line + 1;
		return UL;
	case '`':
		for (i = 0; i < 3; ++i)
			if (line[i] != '`')
				return NORMAL;
		return CODEBACK;
	default:
		if (isdigit(line[0])) {
			for (i = 0; isdigit(line[i]); ++i) ;
			if (line[i] == '.' || line[i] == ')') {
				*contentret = line + i + 1;
				return OL;
			}
		}
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
		fwrite(buff, sizeof(*buff), writelen, outfile);
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
		fwrite(buff, sizeof(*buff), writelen, outfile);
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
			fputs(buff, outfile);

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
