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

#include <util.h>
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
	{'"', "&quot;"},
};

static int writecodespan(char *data, int i, size_t len, FILE *out);
static int writelink(char *data, int i, size_t len, FILE *out);
static int writeimage(char *data, int i, size_t len, FILE *out);
static int writeautolink(char *data, int i, size_t len, FILE *out);
static int writehardbreak(char *data, int i, size_t len, FILE *out);
static int getlinkinfo(char *data, int i, size_t len,
		int *textstart, int *textend,
		int *titlestart, int *titleend,
		int *linkstart, int *linkend);
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
		if ((newi = writelink(data, i, len, out)) >= 0)
			goto special;
		if ((newi = writeimage(data, i, len, out)) >= 0)
			goto special;
		if ((newi = writeautolink(data, i, len, out)) >= 0)
			goto special;
		if ((newi = writehardbreak(data, i, len, out)) >= 0)
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

static int writelink(char *data, int i, size_t len, FILE *out) {
	int textstart, textend, titlestart, titleend, linkstart, linkend;
	i = getlinkinfo(data, i, len, &textstart, &textend,
			&titlestart, &titleend,
			&linkstart, &linkend);
	if (i < 0)
		return -1;
	fputs("<a href='", out);
	writeescaped(data + linkstart, linkend - linkstart, out);
	fputc('\'', out);
	if (titlestart >= 0) {
		fputs(" title='", out);
		writeescaped(data + titlestart, titleend - titlestart, out);
		fputc('\'', out);
	}
	fputc('>', out);
	writeescaped(data + textstart, textend - textstart, out);
	fputs("</a>", out);
	return i;
}

static int writeimage(char *data, int i, size_t len, FILE *out) {
	int textstart, textend, titlestart, titleend, linkstart, linkend;
	if (data[i++] != '!')
		return -1;
	i = getlinkinfo(data, i, len, &textstart, &textend,
			&titlestart, &titleend,
			&linkstart, &linkend);
	if (i < 0)
		return -1;
	fputs("<img src='", out);
	writeescaped(data + linkstart, linkend - linkstart, out);
	fputc('\'', out);
	if (titlestart >= 0) {
		fputs(" title='", out);
		writeescaped(data + titlestart, titleend - titlestart, out);
		fputc('\'', out);
	}
	fputs(" alt='", out);
	writeescaped(data + textstart, textend - textstart, out);
	fputs("'>", out);
	return i;
}

static int writeautolink(char *data, int i, size_t len, FILE *out) {
	int j, linkstart, linkend;
	if (data[i++] != '<')
		return -1;
	linkstart = i;
	if (!isalpha(data[i]))
		return -1;
	for (j = 1; j < 32 && i + j < len; ++j) {
		char c;
		c = data[i + j];
		if (!isalpha(c) && !isdigit(c) && strchr("+.-", c) == NULL)
			break;
	}
	if ((i += j) >= len)
		return -1;
	if (data[i] != ':')
		return -1;
	for (++i; i < len; ++i) {
		if (isctrl(data[i]) || strchr(" <>", data[i]) != NULL)
			break;
	}
	if (i >= len || data[i] != '>')
		return -1;
	linkend = i++;

	fputs("<a href='", out);
	writeescaped(data + linkstart, linkend - linkstart, out);
	fputs("'>", out);
	writeescaped(data + linkstart, linkend - linkstart, out);
	fputs("</a>", out);

	return i;
}

static int writehardbreak(char *data, int i, size_t len, FILE *out) {
	const char *endcode = "  \n";
	const int codelen = strlen(endcode);
	if (data[i] == '\\') {
		if (++i >= len)
			return -1;
		if (data[i++] != '\n')
			return -1;
		fputs("<br />", out);
		return i;
	}
	if (i + codelen >= len)
		return -1;
	if (memcmp(data + i, endcode, codelen) != 0)
		return -1;
	fputs("<br />", out);
	return i + codelen;
}

static int getlinkinfo(char *data, int i, size_t len,
		int *textstart, int *textend,
		int *titlestart, int *titleend,
		int *linkstart, int *linkend) {
	int count;
	enum {
		INITIAL,
		GETTEXT,
		GETDESTSTART,
		GETDESTDETERMINE,
		GETDESTPOINTY,
		GETDESTNORMAL,
		GETTITLEDETERMINE,
		GETTITLEDOUBLEQUOTE,
		GETTITLESINGLEQUOTE,
		GETTITLEPAREN,
		NEARLYTHERE,
		DONE
	} state;
	state = INITIAL;
	for (; i < len; ++i) {
		switch (state) {
		case INITIAL:
			if (data[i] != '[')
				return -1;
			state = GETTEXT;
			count = 1;
			*textstart = i + 1;
			break;
		case GETTEXT:
			if (data[i] == '[')
				++count;
			if (data[i] == ']')
				--count;
			if (count == 0) {
				*textend = i;
				*linkstart = i;
				state = GETDESTSTART;
			}
			break;
		case GETDESTSTART:
			if (data[i] != '(')
				return -1;
			state = GETDESTDETERMINE;
			break;
		case GETDESTDETERMINE:
			if (data[i] == '<') {
				state = GETDESTPOINTY;
				*linkstart = i + 1;
			}
			else {
				state = GETDESTNORMAL;
				*linkstart = i--;
				count = 0;
			}
			break;
		case GETDESTPOINTY:
			if (data[i] == '<' && data[i - 1] != '\\')
				return -1;
			if (data[i] == '>' && data[i - 1] != '\\') {
				*linkend = i;
				state = GETTITLEDETERMINE;
			}
			break;
		case GETDESTNORMAL:
			if (data[i] == '(')
				++count;
			if (data[i] == ')')
				--count;
			if (count < 0) {
				state = GETTITLEDETERMINE;
				*linkend = i--;
				break;
			}
			if (count != 0)
				break;
			if (isctrl(data[i]) || data[i] == ' ') {
				state = GETTITLEDETERMINE;
				*linkend = i;
			}
			break;
		case GETTITLEDETERMINE:
			switch (data[i]) {
			case '"':
				state = GETTITLEDOUBLEQUOTE;
				*titlestart = i + 1;
				break;
			case '\'':
				state = GETTITLESINGLEQUOTE;
				*titlestart = i + 1;
				break;
			case '(':
				state = GETTITLEPAREN;
				count = 1;
				*titlestart = i + 1;
				break;
			default:
				--i;
				*titlestart = *titleend = -1;
				state = NEARLYTHERE;
			}
			break;
		case GETTITLEDOUBLEQUOTE:
			if (data[i] == '"' && data[i - 1] != '\\') {
				*titleend = i;
				state = NEARLYTHERE;
			}
			break;
		case GETTITLESINGLEQUOTE:
			if (data[i] == '\'' && data[i - 1] != '\\') {
				*titleend = i;
				state = NEARLYTHERE;
			}
			break;
		case GETTITLEPAREN:
			if (data[i] == '(' || data[i] == ')') {
				if (data[i - 1] != '\\') {
					*titleend = i;
					state = NEARLYTHERE;
				}
			}
			break;
		case NEARLYTHERE:
			if (data[i] != ')')
				return -1;
			state = DONE;
			break;
		case DONE:
			goto done;
		}
	}
done:
	if (state != DONE)
		return -1;
	return i;
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
