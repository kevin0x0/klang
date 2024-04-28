CC = gcc
OPTIMIZE = -g
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/value/*.c $(ROOT_DIR)src/mm/klgc.c $(ROOT_DIR)src/mm/klmm.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../

.PHONY : clean

clean :
	$(RM) *.exe

