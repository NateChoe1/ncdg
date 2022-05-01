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

#include <io.h>
#include <util.h>
#include <mdutil.h>
#include <inlines.h>
#include <template.h>

struct parsestate {
	struct linedata prev;
	struct string *para;

	int isfirst;
	int intensity;
	/* Similar to the intensity field in the linedata struct. Currently
	 * stores the number of backticks used in FENCECODE.*/
};

static int parseline(char *line, struct parsestate *currstate, FILE *out);
static int endpara(struct parsestate *state, FILE *out);
static void handlehtmlcase(struct linedata *data, struct parsestate *state,
		char *line, FILE *out);
static void handlehtmlmiddle(struct linedata *data, struct parsestate *state,
		char *line, FILE *out);

int parsetemplate(FILE *infile, FILE *outfile) {
	struct linefile *realin;
	struct parsestate currstate;
	int code;

	currstate.prev.type = EMPTY;
	currstate.para = newstring();

	realin = newlinefile(infile);
	for (;;) {
		char *currline;
		currline = getline(realin);
		if (currline == NULL) {
			code = 0;
			break;
		}
		if (parseline(currline, &currstate, outfile)) {
			code = 1;
			break;
		}
	}
	endpara(&currstate, outfile);
	freelinefile(realin);

	return code;
}

static int parseline(char *line, struct parsestate *currstate, FILE *out) {
	struct linedata type;

	identifyline(line, &currstate->prev, &type);

	switch (currstate->prev.type) {
	case FENCECODE:
		if (type.type == FENCECODE &&
				type.data.intensity >=
				currstate->intensity) {
			currstate->prev.type = EMPTY;
			fputs("</code></pre>", out);
			return 0;
		}
		fprintf(out, "%s\n", line);
		currstate->isfirst = 0;
		return 0;
	case HTMLCONCRETE:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case COMMENTLONG:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case PHP:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case COMMENTSHORT:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case CDATA:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case SKELETON:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case GENERICTAG:
		handlehtmlmiddle(&type, currstate, line, out);
		return 0;
	case EMPTY: case PLAIN: case SPACECODE: case HR:
	case SETEXT1: case SETEXT2: case HEADER:
		break;
	}

	switch (type.type) {
	case EMPTY:
		endpara(currstate, out);
		currstate->prev.type = EMPTY;
		break;
	case SETEXT1:
		if (currstate->prev.type != PLAIN)
			return 1;
		currstate->prev.type = EMPTY;
		fputs("<h1>", out);
		writedata(currstate->para->data, currstate->para->len, out);
		fputs("</h1>", out);
		resetstring(currstate->para);
		break;
	case SETEXT2:
		if (currstate->prev.type != PLAIN)
			goto hr;
		currstate->prev.type = EMPTY;
		fputs("<h2>", out);
		writedata(currstate->para->data, currstate->para->len, out);
		fputs("</h2>", out);
		resetstring(currstate->para);
		break;
	case HR: hr:
		endpara(currstate, out);
		currstate->prev.type = EMPTY;
		fputs("<hr>", out);
		break;
	case PLAIN:
		if (currstate->prev.type != PLAIN) {
			endpara(currstate, out);
			currstate->prev.type = PLAIN;
		}
		else
			appendcharstring(currstate->para, '\n');
		appendstrstring(currstate->para, realcontent(line, &type));
		break;
		/* According to the commonmark spec, this markdown:

		Chapter 1
		---

		 * Should NOT compile to this:

		<p>Chapter 1</p><hr>

		 * but rather to this

		<h2>Chapter 1</h2>

		 * This means that we need to store the contents of the
		 * paragraph and only write after obtaining the whole thing
		 * as to not include the wrong tags.
		 * */
	case FENCECODE:
		fputs("<pre><code>", out);
		currstate->prev.type = FENCECODE;
		currstate->isfirst = 1;
		currstate->intensity = type.data.intensity;
		break;
	case SPACECODE:
		if (currstate->prev.type != SPACECODE) {
			endpara(currstate, out);
			currstate->prev.type = SPACECODE;
			fputs("<code class='block'>", out);
		}
		else
			fputs("<br>", out);
		fputs(realcontent(line, &type), out);
		break;
	case HEADER:
		endpara(currstate, out);
		fprintf(out, "<h%d>", type.data.intensity);
		writeline(realcontent(line, &type), out);
		fprintf(out, "</h%d>", type.data.intensity);
		currstate->prev.type = EMPTY;
		break;
	case HTMLCONCRETE:
		handlehtmlcase(&type, currstate, line, out);
		break;
	case COMMENTLONG:
		handlehtmlcase(&type, currstate, line, out);
		break;
	case PHP:
		handlehtmlcase(&type, currstate, line, out);
		break;
	case COMMENTSHORT:
		handlehtmlcase(&type, currstate, line, out);
		break;
	case CDATA:
		handlehtmlcase(&type, currstate, line, out);
		break;
	case SKELETON:
		handlehtmlcase(&type, currstate, line, out);
		break;
	case GENERICTAG:
		handlehtmlcase(&type, currstate, line, out);
		break;
	}
	return 0;
}

static int endpara(struct parsestate *state, FILE *out) {
	switch (state->prev.type) {
	case EMPTY: case HR:
	case HTMLCONCRETE: case COMMENTLONG: case PHP: case COMMENTSHORT:
	case CDATA: case SKELETON: case GENERICTAG:
		return 0;
	case PLAIN:
		fputs("<p>", out);
		writedata(state->para->data, state->para->len, out);
		fputs("</p>", out);
		resetstring(state->para);
		return 0;
	case SPACECODE: case FENCECODE:
		fputs("</code>", out);
		return 0;
	case HEADER:
		fprintf(out, "</h%d>", state->prev.data.intensity);
		return 0;
	case SETEXT1:
		fputs("</h1>", out);
		break;
	case SETEXT2:
		fputs("</h2>", out);
		break;
	}
	return 1;
}

static void handlehtmlcase(struct linedata *data, struct parsestate *state,
		char *line, FILE *out) {
	endpara(state, out);
	fputs(line, out);
	fputc('\n', out);
	state->prev.type = data->type;
}

static void handlehtmlmiddle(struct linedata *data, struct parsestate *state,
		char *line, FILE *out) {
	if (state->prev.type == data->type && !data->data.isfirst) {
		state->prev.type = EMPTY;
		return;
	}
	fputs(line, out);
	fputc('\n', out);
}
