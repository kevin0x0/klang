CC = gcc
OPTIMIZE = -g
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/value/*.c $(ROOT_DIR)src/objpool/*.c $(ROOT_DIR)src/gc/gc.c $(ROOT_DIR)src/mm/mm.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -L $(ROOT_DIR)../utils/lib/ -lutils

.PHONY : clean

clean :
	$(RM) *.exe

