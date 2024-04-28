CC = gcc
OPTIMIZE = -O3
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/misc/*.c $(ROOT_DIR)src/parse/*.c $(ROOT_DIR)src/cst/*.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -g -Wall -Wextra -Winline -pedantic -DNDEBUG -L $(ROOT_DIR)../utils/lib/ -lutils

.PHONY : clean

clean :
	$(RM) test.exe
