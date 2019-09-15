PREFIX ?= /usr
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib
.PHONY: install

canvas.o: canvas.c
	gcc -c $^ -o $@

canvas.a: canvas.o
	ar rcs $@ $^

install: canvas.h canvas.a
	cp canvas.h $(INCLUDEDIR)
	cp canvas.a $(LIBDIR)
