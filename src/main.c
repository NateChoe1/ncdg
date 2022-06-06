#include <stdio.h>
#include <stdlib.h>

#include <parse.h>

static void printhelp(char *progname);

int main(int argc, char **argv) {
	char *template;
	FILE *outfile;

	if (argc < 2)
		printhelp(argv[0]);
	template = argv[1];

	if (argc < 3)
		outfile = stdout;
	else {
		outfile = fopen(argv[2], "w");
		if (outfile == NULL) {
			fprintf(stderr, "Failed to open file %s for writing\n",
					argv[2]);
		}
	}

	return parsefile(template, outfile);
}

static void printhelp(char *progname) {
	fprintf(stderr, "Usage: %s [template] (out)\n", progname);
	exit(EXIT_FAILURE);
}
