CC = gcc
OPTIMIZE =
ROOT_DIR = $(abspath ../../)/

test : test.c $(ROOT_DIR)src/code/*.c $(ROOT_DIR)src/misc/*.c $(ROOT_DIR)src/parse/*.c $(ROOT_DIR)src/cst/*.c $(ROOT_DIR)src/error/*.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR) -I $(ROOT_DIR)deps/k/ -g -Wall -Wextra -Winline -pedantic -L $(ROOT_DIR)deps/k/lib/ -lk -fsanitize=address

test.s : test.c
	$(CC) -S -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -Wall -Wextra -Winline -pedantic -DNDEBUG

.PHONY : clean

clean :
	$(RM) test.exe
