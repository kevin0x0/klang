CC = gcc
OPTIMIZE = -O2
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/value/*.c $(ROOT_DIR)src/vm/*.c $(ROOT_DIR)src/mm/*.c $(ROOT_DIR)src/misc/klassert.c $(ROOT_DIR)src/klapi.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -g -Wall -Wextra

.PHONY : clean

clean :
	$(RM) test.exe
