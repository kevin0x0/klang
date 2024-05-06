CC = gcc
OPTIMIZE = -O2
ROOT_DIR = $(abspath ../..)/
MEMORY_CHECK = -fsanitize=address

test : test.c
	$(CC) -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR) -I $(ROOT_DIR)deps/k/ -g -Wall -Wextra -Winline -L $(ROOT_DIR)deps/k/lib/ -L $(ROOT_DIR)lib/ -Wl,-Bstatic -lklang -Wl,-Bdynamic -DNDEBUG

test.s : test.c
	$(CC) -S -o $@ $^ $(OPTIMIZE) -I $(ROOT_DIR)../ -Wall -Wextra -Winline -pedantic -DNDEBUG

.PHONY : clean

clean :
	$(RM) test.exe
