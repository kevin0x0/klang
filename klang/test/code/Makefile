CC = gcc
OPTIMIZE = -O3
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/code/*.c $(ROOT_DIR)src/misc/*.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -g -Wall -Wextra -Winline -pedantic -DNDEBUG

test.s : test.c
	$(CC) -S -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -Wall -Wextra -Winline -pedantic -DNDEBUG

.PHONY : clean

clean :
	$(RM) test.exe
