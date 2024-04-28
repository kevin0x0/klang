CC = gcc
OPTIMIZE = -g
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/parse/klsymtab.c $(ROOT_DIR)src/objpool/*.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -L $(ROOT_DIR)../utils/lib/ -lutils

.PHONY : clean

clean :
	$(RM) *.exe

