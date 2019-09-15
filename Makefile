PREFIX ?= /usr
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
DEFAULT: canvas.a
.PHONY: install clean

canvas.o: canvas.c
	$(CC) -g -O2 -Wall -c $^ -o $@

libcanvas.a: canvas.o
	ar rcs $@ $^

install: canvas.h libcanvas.a
	cp canvas.h $(INCLUDEDIR)
	cp libcanvas.a $(LIBDIR)

clean:
	rm -f *.a *.o
