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
#include <stdlib.h>
#include <string.h>

#include <util.h>

struct string *newstring() {
	struct string *ret;
	ret = malloc(sizeof *ret);
	if (ret == NULL)
		return NULL;
	ret->len = 0;
	ret->alloc = 20;
	ret->data = malloc(ret->alloc);
	if (ret->data == NULL)
		return NULL;
	return ret;
}

void freestring(struct string *str) {
	free(str->data);
	free(str);
}

int appendcharstring(struct string *str, char c) {
	if (str->len >= str->alloc) {
		char *newdata;
		size_t newalloc;
		newalloc = str->alloc * 2;
		newdata = realloc(str->data, newalloc);
		if (newdata == NULL) {
			return 1;
		}
		str->data = newdata;
		str->alloc = newalloc;
	}
	str->data[str->len++] = c;
	return 0;
}

int appendstrstring(struct string *str, char *s) {
	size_t len;
	len = strlen(s);
	if (str->len + len >= str->alloc) {
		char *newdata;
		size_t newalloc;
		newalloc = str->alloc;
		while (str->len + len >= newalloc)
			newalloc *= 2;
		newdata = realloc(str->data, newalloc);
		if (newdata == NULL)
			return 1;
		str->data = newdata;
		str->alloc = newalloc;
	}
	memcpy(str->data + str->len, s, len);
	str->len += len;
	return 0;
}

void resetstring(struct string *str) {
	str->len = 0;
}
