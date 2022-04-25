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
#include <string.h>

#include <util.h>
#include <mdutil.h>

static char *truncate(char *str);
static char *after(char *begin, char *str);
static void identifyend(char *line, enum linetype prev, struct linedata *ret);

static char *concretetags[] = { "pre", "script", "style", "textarea" };
static char *skeletontags[] = {
	"address", "article", "aside", "base", "basefont", "blockquote", "body",
	"caption", "center", "col", "colgroup", "dd", "details", "dialog",
	"dir", "div", "dl", "dt", "fieldset", "figcaption", "figure", "footer",
	"form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", "h6", "head",
	"header", "hr", "html", "iframe", "legend", "li", "link", "main",
	"menu", "menuitem", "nav", "noframes", "ol", "optgroup", "option", "p",
	"param", "section", "source", "summary", "table", "tbody", "td",
	"tfoot", "th", "thead", "title", "tr", "track", "ul",
};

void identifyline(char *line, struct linedata *prev, struct linedata *ret) {
	int i;
	if (HTMLSTART <= prev->type && prev->type <= HTMLEND) {
		identifyend(truncate(line), prev->type, ret);
		return;
	}
	if (prev->type != PLAIN) {
		for (i = 0; i < 4; ++i) {
			if (!isspace(line[i]))
				goto notspacecode;
		}
		ret->type = SPACECODE;
		return;
	}
notspacecode:
	line = truncate(line);
	if (line[0] == '\0') {
		ret->type = EMPTY;
		return;
	}
	{
		int hrcount;
		if (strchr("-*_=", line[0]) == NULL)
			goto nothr;
		/* A delimiting line can only contain '-', '*', '_', and ' '. */
		hrcount = 0;
		for (i = 0; line[i]; ++i) {
			if (!isspace(line[i]) && line[i] != line[0])
				goto nothr;
			/* You can't mix delimiter characters, and you can't
			 * have anything other than a delimiter character or
			 * white space. */
			if (line[i] == line[0])
				++hrcount;
		}
		if (hrcount >= 3) {
			switch (line[0]) {
			case '=':
				ret->type = SETEXT1;
				return;
			case '-':
				ret->type = SETEXT2;
				return;
			default:
				ret->type = HR;
				return;
			}
		}
		/* There has to be at least 3 delimiter characters */
	}
nothr:
	for (i = 0; line[i] == '`'; ++i) ;
	if (i >= 3) {
		ret->type = FENCECODE;
		ret->data.intensity = i;
		/* The last line of a fenced code block must have at least the
		 * same number of backticks as the first. */
		return;
	}
/* notfencedcode: */

	if (line[0] == '#') {
		int pcount;
		for (pcount = 0; line[pcount] == '#'; ++pcount) ;
		if (line[pcount] != ' ' && line[pcount] != '\0')
			goto notheader;
		ret->type = HEADER;
		ret->data.intensity = pcount;
		return;
	}

notheader:

#define HTMLSTARTCASE(start, rettype) \
	if (after(start, line) != NULL) { \
		ret->type = rettype; \
		ret->data.isfirst = 1; \
		return; \
	}
	HTMLSTARTCASE("<!--", COMMENTLONG);
	HTMLSTARTCASE("<![CDATA[", CDATA);
	HTMLSTARTCASE("<?", PHP);
	HTMLSTARTCASE("<!", COMMENTSHORT);

	if (line[0] == '<') {
		char *testline;
		int isopen;
		testline = line + 1;
		for (i = 0; i < LEN(concretetags); ++i) {
			char *aftertag;
			aftertag = after(concretetags[i], testline);
			if (aftertag == NULL)
				continue;
			if (aftertag[0] == '\0' || strchr(" >", aftertag[0])) {
				ret->type = HTMLCONCRETE;
				ret->data.isfirst = 1;
				return;
			}
		}
		if (testline[0] == '/') {
			++testline;
			isopen = 0;
		}
		else
			isopen = 1;
		for (i = 0; i < LEN(skeletontags); ++i) {
			char *aftertag;
			aftertag = after(skeletontags[i], testline);
			if (aftertag == NULL)
				continue;
			if (aftertag[0] == '\0' ||
					strchr(" >", aftertag[0]) ||
					after("/>", aftertag) != NULL) {
				ret->type = SKELETON;
				ret->data.isfirst = 1;
				return;
			}
		}

		/* < || </ */

		if (!isalpha(testline[0]))
			goto nothtml;
		++testline;
		while (isalnum(testline[0]) || testline[0] == '-')
			++testline;

		/* <[tag name] || </[tag name] */

		for (;;) {
			char *newtestline;
			newtestline = truncate(testline);

			if (!isalpha(newtestline[0]) &&
					strchr("_:", newtestline[0]) == NULL)
				break;
			++newtestline;
			while (isalnum(newtestline[0]) ||
					strchr("_.:-", newtestline[0]) != NULL)
				++newtestline;
			/* Swallow attribute name */

			newtestline = truncate(newtestline);
			if (newtestline[0] == '=') {
				char start;
				++newtestline;
				newtestline = truncate(newtestline);
				start = newtestline[0];
				switch (start) {
				case '\'': case '"':
					while (newtestline[0] != start &&
							newtestline[0] != '\0')
						++newtestline;
					if (newtestline[0] == '\0')
						goto nothtml;
					break;
				/* Swallow single/double quoted attribute
				 * value */
				default:
					while (strchr("\"'=<>`", newtestline[0])
							== NULL &&
							newtestline[0] != '\0')
						++newtestline;
					if (newtestline[0] == '\0')
						goto nothtml;
					break;
				/* Swallow unquoted attribute value */
				}
			}
			/* Swallow attribute value */

			testline = newtestline;
		}
		
		/* <[tag name][attribute]* || </tag name][attribute]* */
		if (isopen && testline[0] == '/')
			++testline;
		if (testline[0] == '>') {
			ret->type = GENERICTAG;
			ret->data.isfirst = 1;
			return;
		}
	}
nothtml:

	ret->type = PLAIN;
	return;
}

char *realcontent(char *line, struct linedata *data) {
	switch (data->type) {
	case EMPTY: case HR: case SETEXT1: case SETEXT2: case FENCECODE:
	case HTMLCONCRETE: case COMMENTLONG: case PHP: case CDATA:
	case SKELETON: case COMMENTSHORT: case GENERICTAG:
		return NULL;
	case PLAIN:
		return line;
	case SPACECODE:
		return line + 4;
	case HEADER:
		return truncate(line + data->data.intensity);
	}
	return NULL;
}

static char *truncate(char *str) {
	while (isspace(str[0]))
		++str;
	return str;
}

static char *after(char *begin, char *str) {
	int i;
	for (i = 0; begin[i]; ++i) {
		if (begin[i] != str[0])
			return NULL;
		++str;
	}
	return str;
}

static void identifyend(char *line, enum linetype prev, struct linedata *ret) {
	int i;
	ret->type = EMPTY;
	switch (prev) {
	case EMPTY: case PLAIN: case SPACECODE: case FENCECODE: case HR:
	case SETEXT1: case SETEXT2: case HEADER:
		return;
	/* In this case, something has gone terribly wrong. */

	case HTMLCONCRETE:
		for (i = 0; i < LEN(concretetags); ++i) {
			char endtag[30];
			sprintf(endtag, "</%s>", concretetags[i]);
			if (strstr(line, endtag) != NULL) {
				ret->type = HTMLCONCRETE;
				ret->data.isfirst = 0;
				return;
			}
		}
		return;
	case COMMENTLONG:
		if (strstr(line, "-->") != NULL) {
			ret->type = COMMENTLONG;
			ret->data.isfirst = 0;
		}
		return;
	case PHP:
		if (strstr(line, "?>") != NULL) {
			ret->type = PHP;
			ret->data.isfirst = 0;
		}
		return;
	case COMMENTSHORT:
		if (strchr(line, '>') != NULL) {
			ret->type = COMMENTSHORT;
			ret->data.isfirst = 0;
		}
		return;
	case CDATA:
		if (strstr(line, "]]>") != NULL) {
			ret->type = CDATA;
			ret->data.isfirst = 0;
		}
		return;
	case SKELETON:
		if (line[0] == '\0') {
			ret->type = SKELETON;
			ret->data.isfirst = 0;
		}
		return;
	case GENERICTAG:
		if (line[0] == '\0') {
			ret->type = GENERICTAG;
			ret->data.isfirst = 0;
		}
		return;
	}
}
