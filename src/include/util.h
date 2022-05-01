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
#ifndef HAVE_UTIL
#define HAVE_UTIL

#include <stddef.h>

#define LEN(arr) (sizeof(arr) / sizeof *(arr))

struct string {
	size_t len;
	size_t alloc;
	char *data;
};

struct string *newstring();
void freestring(struct string *str);
int appendcharstring(struct string *str, char c);
int appendstrstring(struct string *str, char *s);
void resetstring(struct string *str);
int isctrl(int c);

#endif
