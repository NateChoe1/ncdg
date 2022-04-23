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
#include <template.h>

static void printhelp(FILE *file, char *name);

int main(int argc, char **argv) {
	char *header, *template, *footer, *out;
	FILE *outfile;
	int c;
	const char *filefailmsg = "Failed to open file %s\n";

	header = template = footer = out = NULL;

	while ((c = getopt(argc, argv, "b:t:e:o:h")) >= 0) {
		switch (c) {
		case 'b':
			header = optarg;
			break;
		case 't':
			template = optarg;
			break;
		case 'e':
			footer = optarg;
			break;
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

	if (header != NULL) {
		FILE *headerfile;
		headerfile = fopen(header, "r");
		if (headerfile != NULL) {
			if (copyhtml(headerfile, outfile)) {
				fputs(filefailmsg, stderr);
				return 1;
			}
		}
		else {
			fprintf(stderr, filefailmsg, header);
			return 1;
		}
	}
	if (template != NULL) {
		FILE *templatefile;
		templatefile = fopen(template, "r");
		if (templatefile != NULL) {
			if (parsetemplate(templatefile, outfile))
				return 1;
		}
		else {
			fprintf(stderr, filefailmsg, template);
			return 1;
		}
	}
	if (footer != NULL) {
		FILE *footerfile;
		footerfile = fopen(footer, "r");
		if (footerfile != NULL) {
			if (copyhtml(footerfile, outfile)) {
				fputs("Failed to copy footer\n", stderr);
				return 1;
			}
		}
		else {
			fprintf(stderr, filefailmsg, footer);
			return 1;
		}
	}
	if (outfile != stdout)
		fclose(outfile);
	return 0;
}

static void printhelp(FILE *file, char *name) {
	fprintf(file,
"Usage: %s -b [header] -t [template] -e [footer] -o [output]\n", name);
	fputs(
"This program is free software. You can redistribute and/or modify it under\n"
"the terms of the GNU General Public License as published by the Free\n"
"Software Foundation, either version 3 of the License, or (at your option)\n"
"any later version.\n", file);
}
