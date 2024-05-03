CC = gcc
OPTIMIZE = -O2
ROOT_DIR = $(abspath ../..)/

test : test.c $(ROOT_DIR)src/value/*.c $(ROOT_DIR)src/vm/*.c $(ROOT_DIR)src/mm/*.c $(ROOT_DIR)src/misc/*.c $(ROOT_DIR)src/klapi.c $(ROOT_DIR)src/lang/klconvert.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR) -g -Wall -Wextra -Winline -pedantic -DNDEBUG -fno-unroll-all-loops

test.s : test.c
	$(CC) -S -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -Wall -Wextra -Winline -pedantic -DNDEBUG

.PHONY : clean

clean :
	$(RM) test.exe
