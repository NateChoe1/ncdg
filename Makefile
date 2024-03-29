SRC = $(wildcard src/*.c)
OBJ = $(subst .c,.o,$(subst src,work,$(SRC)))
LDFLAGS =
CFLAGS := -pipe -Wall -Wpedantic -Wshadow -ansi -ggdb
CFLAGS += -Isrc/include/
INSTALLDIR := /usr/bin/
OUT = ncdg

build/$(OUT): $(OBJ)
	$(CC) $(OBJ) -o build/$(OUT) $(LDFLAGS)

work/%.o: src/%.c $(wildcard src/include/*.h)
	$(CC) $(CFLAGS) $< -c -o $@

install: build/$(OUT)
	cp build/$(OUT) $(INSTALLDIR)/$(OUT)

uninstall: $(INSTALLDIR)/$(OUT)
	rm $(INSTALLDIR)/$(OUT)
