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
#include <mdutil.h>
#include <inlines.h>

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

struct deliminfo {
	int length;
	unsigned int capabilities;
	char c;
};
static const unsigned int canopen = 1 << 0;
static const unsigned int canclose = 1 << 1;

static int writecodespan(char *data, int i, size_t len, FILE *out);
static int writelink(char *data, int i, size_t len, FILE *out);
static int writeimage(char *data, int i, size_t len, FILE *out);
static int writeautolink(char *data, int i, size_t len, FILE *out);
static int writehardbreak(char *data, int i, size_t len, FILE *out);
static int writerawhtml(char *data, int i, size_t len, FILE *out);
static int writedelimrun(char *data, int i, size_t len, FILE *out);
static int getdeliminfo(char *data, int i, size_t len, struct deliminfo *ret);
/* Returns non-zero if i isn't a delimiter */
static void writeopentags(int count, FILE *out);
static void writeclosetags(int count, FILE *out);
static int getlinkinfo(char *data, int i, size_t len,
		int *textstart, int *textend,
		int *titlestart, int *titleend,
		int *linkstart, int *linkend);
static void writeescaped(char *data, size_t len, FILE *out);
static void writechescape(char c, FILE *out);
static int inlinehtmlcase(char *data, char *start, char *end, FILE *out);
/* Returns length of raw html */

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
		if ((newi = writerawhtml(data, i, len, out)) >= 0)
			goto special;
		if ((newi = writedelimrun(data, i, len, out)) >= 0)
			goto special;
		if (data[i] == '\\') {
			if (!ispunc(data[i + 1]))
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

static int writerawhtml(char *data, int i, size_t len, FILE *out) {
	int taglen;
	data += i;
	taglen = isgenerictag(data);
	if (taglen > 0) {
		fwrite(data, 1, taglen, out);
		return i + taglen;
	}

	if (memcmp(data, "<!--", 4) == 0) {
		char *end, *text;
		int j;
		text = data + 4;
		end = strstr(text, "-->");
		if (end == NULL)
			goto notcomment;
		taglen = end - text;
		if (text[0] == '>' || memcmp(text, "->", 2) == 0)
			goto notcomment;
		if (text[taglen - 1] == '-') {
			goto notcomment;
		}
		for (j = 0; j < taglen - 1; ++j) {
			if (memcmp(text + j, "--", 2) == 0)
				goto notcomment;
		}

		fwrite(data, 1, taglen + 7, out);

		return i + taglen + 7;
	}
notcomment:
	if ((taglen = inlinehtmlcase(data, "<?", "?>", out)) > 0)
		return i + taglen;
	if ((taglen = inlinehtmlcase(data, "<!", ">", out)) > 0)
		return i + taglen;
	if ((taglen = inlinehtmlcase(data, "<![CDATA[", "]]>", out)) > 0)
		return i + taglen;

	return -1;
}

static int writedelimrun(char *data, int i, size_t len, FILE *out) {
	struct deliminfo startdelim;
	int end;
	if (getdeliminfo(data, i, len, &startdelim))
		return -1;
	for (end = i + startdelim.length;
			end < len && startdelim.length > 0; ++end) {
		struct deliminfo enddelim;
		if (getdeliminfo(data, end, len, &enddelim))
			continue;
		if (startdelim.c != enddelim.c)
			continue;
		if (enddelim.capabilities & canclose) {
			if (startdelim.length == enddelim.length) {
				int begin;
				begin = i + startdelim.length;
				writeopentags(startdelim.length, out);
				writedata(data + begin, end - begin, out);
				writeclosetags(startdelim.length, out);
				return end + enddelim.length;
			}
		}
		else if (enddelim.capabilities & canopen)
			startdelim.length -= enddelim.length;
		/* Handles cases like "**a b* c* "*/
		/* TODO: This does not properly handle cases like "**a b* c* "*/
	}
	return -1;
}

static int getdeliminfo(char *data, int i, size_t len, struct deliminfo *ret) {
	const unsigned int flanksleft = 1 << 0;
	const unsigned int flanksright = 1 << 1;
	unsigned int flanks;
	if (data[i] != '*' && data[i] != '_')
		return 1;
	ret->c = data[i];
	for (ret->length = 0; i + ret->length < len &&
			data[i + ret->length] == ret->c; ++ret->length) ;
	flanks = 0;
	if (i != 0) {
		if (ispunc(data[i - 1])) {
			if (i + ret->length >= len)
				flanks |= flanksright;
			else {
				char after;
				after = data[i + ret->length];
				if (isspace(after) || ispunc(after))
					flanks |= flanksright;
			}
		}
		else {
			flanks |= flanksright;
		}
	}
	if (i + ret->length < len) {
		if (!isspace(data[i + ret->length])) {
			if (!ispunc(data[i + ret->length]))
				flanks |= flanksleft;
			else if (i == 0 || isspace(data[i - 1]) || ispunc(data[i - 1]))
				flanks |= flanksleft;
		}
	}

	ret->capabilities = 0;
	if (ret->c == '*') {
		if (flanks & flanksleft)
			ret->capabilities |= canopen;
		if (flanks & flanksright)
			ret->capabilities |= canclose;
	}
	else {
		if (flanks & flanksleft && (!(flanks & flanksright) ||
				(i > 0 && ispunc(data[i - 1]))))
			ret->capabilities |= canopen;
		if (flanks & flanksright && (!(flanks & flanksleft) ||
				(ret->length + i < len && ispunc(data[i + 1]))))
			ret->capabilities |= canopen;
	}
	/* I have absolutely no clue why the different characters used in
	 * delimiter runs have different meanings, but I absolutely hate that
	 * they do. */
	return 0;
}

static void writeopentags(int count, FILE *out) {
	int i;
	if (count % 2 != 0)
		fputs("<em>", out);
	for (i = 0; i < count / 2; ++i)
		fputs("<strong>", out);
}
static void writeclosetags(int count, FILE *out) {
	int i;
	for (i = 0; i < count / 2; ++i)
		fputs("</strong>", out);
	if (count % 2 != 0)
		fputs("</em>", out);
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

static int inlinehtmlcase(char *data, char *start, char *end, FILE *out) {
	int i;
	int len;
	char *endloc;
	for (i = 0; start[i] != '\0'; ++i)
		if (data[i] != start[i])
			return 0;
	endloc = strstr(data, end);
	if (endloc == NULL)
		return 0;
	len = endloc - data + strlen(end);
	fwrite(data, 1, len, out);
	return len;
}
