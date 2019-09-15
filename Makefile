PREFIX ?= /usr
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
DEFAULT: canvas.a
.PHONY: install clean

canvas.o: canvas.c
	gcc -g -O2 -Wall -c $^ -o $@

canvas.a: canvas.o
	ar rcs $@ $^

install: canvas.h canvas.a
	cp canvas.h $(INCLUDEDIR)
	cp canvas.a $(LIBDIR)

clean:
	rm -f *.a *.o
