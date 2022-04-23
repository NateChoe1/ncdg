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
#include <string.h>
#include <stddef.h>

#include <mdutil.h>

static char *truncate(char *str);

void identifyline(char *line, enum nodetype prev, struct linedata *ret) {
	int i;
	if (prev != PARAGRAPH) {
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
	for (i = 0; i < 3; ++i) {
		if (line[i] != '`')
			goto notfencedcode;
	}
	ret->type = FENCECODE;
	return;
notfencedcode:

	if (line[0] == '#') {
		int pcount;
		for (pcount = 0; line[pcount] == '#'; ++pcount) ;
		if (line[pcount] != ' ' && line[pcount] != '\0')
			goto notheader;
		ret->type = HEADER;
		ret->intensity = pcount;
		return;
	}

notheader:
	ret->type = PLAIN;
	return;
}
/* TODO: Finish this */

static char *truncate(char *str) {
	while (isspace(str[0]))
		++str;
	return str;
}

char *realcontent(char *line, struct linedata *data) {
	switch (data->type) {
	case EMPTY: case HR: case SETEXT1: case SETEXT2: case FENCECODE:
		return NULL;
	case PLAIN:
		return line;
	case SPACECODE:
		return line + 4;
	case HEADER:
		return truncate(line + data->intensity);
	}
	return NULL;
}
