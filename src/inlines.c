#include <string.h>

#include <inlines.h>

static const char *punctuation = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
struct escape {
	char c;
	char *code;
};
struct escape escapes[] = {
	{'<', "&lt;"},
	{'>', "&gt;"},
	{'&', "&amp;"},
};

static int writecodespan(char *data, int i, size_t len, FILE *out);
static void writeescaped(char *data, size_t len, FILE *out);
static void writechescape(char c, FILE *out);

void writeline(char *data, FILE *out) {
	writedata(data, strlen(data), out);
}

void writedata(char *data, size_t len, FILE *out) {
	int i;
	for (i = 0; i < len; ++i) {
		int newi;
		if ((newi = writecodespan(data, i, len, out)) >= 0)
			goto special;
		if (data[i] == '\\') {
			if (strchr(punctuation, data[i + 1]) == NULL)
				writechescape('\\', out);
			++i;
		}
		writechescape(data[i], out);
		continue;
special:
		i = newi - 1;
	}
}

static int writecodespan(char *data, int i, size_t len, FILE *out) {
	int ticklen;
	int start, end;

	if (data[i] != '`')
		return -1;
	for (ticklen = 0; data[i + ticklen] == '`'; ++ticklen) ;

	start = i + ticklen;
	for (end = start; end < len; ++end) {
		int thislen;
		if (data[end] != '`')
			continue;
		for (thislen = 0; data[end + thislen] == '`'; ++thislen);
		if (ticklen == thislen)
			break;
		end += thislen - 1;
	}
	if (end >= len) {
		for (i = 0; i < ticklen; ++i)
			fputc('`', out);
		return start + ticklen;
	}

	fputs("<code>", out);
	writeescaped(data + start, end - start, out);
	fputs("</code>", out);
	return end + ticklen;
}

static void writeescaped(char *data, size_t len, FILE *out) {
	int i;
	for (i = 0; i < len; ++i)
		writechescape(data[i], out);
}

static void writechescape(char c, FILE *out) {
	int i;
	for (i = 0; i < sizeof escapes / sizeof *escapes; ++i) {
		if (escapes[i].c == c) {
			fputs(escapes[i].code, out);
			return;
		}
	}
	fputc(c, out);
}
