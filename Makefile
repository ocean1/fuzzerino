# Makefile
.PHONY: FORCE all clean


CC = gcc


all: generator parser


generator:
	$(CC) -c g.c -o g.o -Wall
	$(CC) g.o -o generator -Wall

parser:
	@echo "not quite there"

clean:
	rm -f generator
	rm -f *.o
