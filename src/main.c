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
#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <unistd.h>

#include <html.h>
#include <markdown.h>

static void printhelp(FILE *file, char *name);

int main(int argc, char **argv) {
	int i;
	char *out;
	FILE *outfile;
	int c;

	out = NULL;

	while ((c = getopt(argc, argv, "o:h")) >= 0) {
		switch (c) {
		case 'o':
			out = optarg;
			break;
		case 'h':
			printhelp(stdout, argv[0]);
			return 0;
		default:
			printhelp(stderr, argv[0]);
			return 1;
		}
	}

	if (out == NULL)
		outfile = stdout;
	else
		outfile = fopen(out, "w");

	for (i = optind; i < argc; ++i) {
		FILE *infile;
		infile = fopen(argv[i], "r");
		if (infile == NULL) {
			fprintf(stderr, "Failed to open file %s\n", argv[i]);
			return 1;
		}
		if (parsemarkdown(infile, outfile)) {
			fprintf(stderr, "Failed to parse file %s\n", argv[i]);
			return 1;
		}
	}

	return 0;
}

static void printhelp(FILE *file, char *name) {
	fprintf(file,
"Usage: %s [file1.md] [file2.md] ... -o [output]\n", name);
	fputs(
"This program is free software. You can redistribute and/or modify it under\n"
"the terms of the GNU General Public License as published by the Free\n"
"Software Foundation, either version 3 of the License, or (at your option)\n"
"any later version.\n", file);
}
