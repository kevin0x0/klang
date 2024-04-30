CC = gcc
OPTIMIZE = -O3
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/misc/*.c $(ROOT_DIR)src/parse/*.c $(ROOT_DIR)src/cst/*.c $(ROOT_DIR)src/error/*.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR) -I $(ROOT_DIR)deps/k/ -g -Wall -Wextra -Winline -pedantic -L $(ROOT_DIR)deps/k/lib/ -lk

.PHONY : clean

clean :
	$(RM) test.exe
