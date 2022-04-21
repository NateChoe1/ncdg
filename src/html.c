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
#include <stdio.h>
#include <ctype.h>

#include <html.h>

/* truncates groups of whitespace into a single space, removes all whitespace
 * immediately before and after <> characters.
 *
 * Example: " < text  in html>   " -> "<text in html>"
 * */
int copyhtml(FILE *in, FILE *out) {
	int c;
	int addspace;
	for (;;) {
		addspace = 0;
		c = fgetc(in);
		if (c == EOF)
			return 0;
		if (isspace(c)) {
			addspace = 1;
			while (isspace(c)) {
				c = fgetc(in);
				if (c == EOF)
					return 0;
			}
		}
		if (c == '<' || c == '>') {
			fputc(c, out);
			while (isspace(c = fgetc(in))) ;
			ungetc(c, in);
		}
		else {
			if (addspace)
				fputc(' ', out);
			fputc(c, out);
		}
	}
	return 0;
}
