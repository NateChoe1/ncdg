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

#ifndef HAVE_MDUTIL
#define HAVE_MDUTIL

enum linetype {
	EMPTY,
	PLAIN,
	SPACECODE,
	FENCECODE,
	HR,
	SETEXT1,
	/* === */
	SETEXT2,
	/* --- */
	HEADER
};

struct linedata {
	enum linetype type;
	int intensity;
};

enum nodetype {
	PARAGRAPH,
	CODE,
	/* Used for code that starts with spaces */
	CODEBLOCK,
	/* Used for triple backtick code */
	NONE
};

void identifyline(char *line, enum nodetype prev, struct linedata *ret);
/* prev is almost never used, but sometimes it is. */
char *realcontent(char *line, struct linedata *data);

#endif
