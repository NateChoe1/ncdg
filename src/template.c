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
#include <template.h>

struct parsestate {
	enum nodetype type;
	struct string *para;
	int isfirst;
	/* Used to insert <br> tags. Currently onlu used for FENCECODE. */
};

static int parseline(char *line, struct parsestate *currstate, FILE *out);
static int endpara(struct parsestate *state, FILE *out);

int parsetemplate(FILE *infile, FILE *outfile) {
	struct linefile *realin;
	struct parsestate currstate;
	int code;

	currstate.type = NONE;
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
	enum linetype type;

	type = identifyline(line, currstate->type);

	if (currstate->type == CODEBLOCK) {
		if (type == FENCECODE) {
			currstate->type = NONE;
			fputs("</code>", out);
			return 0;
		}
		if (!currstate->isfirst)
			fputs("<br>", out);
		fputs(line, out);
		currstate->isfirst = 0;
		return 0;
	}

	switch (type) {
	case EMPTY:
		endpara(currstate, out);
		currstate->type = NONE;
		return 0;
	case SETEXT1:
		if (currstate->type != PARAGRAPH)
			return 1;
		currstate->type = NONE;
		fputs("<h1>", out);
		fwrite(currstate->para->data, 1, currstate->para->len, out);
		fputs("</h1>", out);
		resetstring(currstate->para);
		return 0;
	case SETEXT2:
		if (currstate->type != PARAGRAPH)
			goto hr;
		currstate->type = NONE;
		fputs("<h2>", out);
		fwrite(currstate->para->data, 1, currstate->para->len, out);
		fputs("</h2>", out);
		resetstring(currstate->para);
		return 0;
	case HR: hr:
		endpara(currstate, out);
		currstate->type = NONE;
		fputs("<hr>", out);
		return 0;
	case PLAIN:
		if (currstate->type != PARAGRAPH) {
			endpara(currstate, out);
			currstate->type = PARAGRAPH;
		}
		else
			appendcharstring(currstate->para, ' ');
		appendstrstring(currstate->para, realcontent(line, type));
		return 0;
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
		fputs("<code class='block'>", out);
		currstate->type = CODEBLOCK;
		currstate->isfirst = 1;
		break;
	case SPACECODE:
		if (currstate->type != CODE) {
			endpara(currstate, out);
			currstate->type = CODE;
			fputs("<code class='block'>", out);
		}
		else
			fputs("<br>", out);
		fputs(realcontent(line, type), out);
		break;
	}
	return 0;
}

static int endpara(struct parsestate *state, FILE *out) {
	switch (state->type) {
	case PARAGRAPH:
		fputs("<p>", out);
		fwrite(state->para->data, 1, state->para->len, out);
		fputs("</p>", out);
		resetstring(state->para);
		return 0;
	case CODE: case CODEBLOCK:
		fputs("</code>", out);
		return 0;
	case NONE:
		return 0;
	}
	return 1;
}
