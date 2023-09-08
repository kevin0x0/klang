CC = gcc
OPTIMIZE = -g
ROOT_DIR = $(abspath $(abspath $(shell pwd))/../../..)/
CFLAGS = -Wall $(OPTIMIZE) -I $(ROOT_DIR)

PARGEN_DIR = $(ROOT_DIR)pargen/
UTILS_DIR = $(ROOT_DIR)utils/

INC_DIR = $(PARGEN_DIR)include/
SRC_DIR = $(PARGEN_DIR)src/
BIN_DIR = $(PARGEN_DIR)bin/

#parser path
PS_INC_DIR = $(INC_DIR)parser/
PS_DIR = $(SRC_DIR)parser/

UTILS_INC = $(UTILS_DIR)include/



test.exe : test.c $(BIN_DIR)error.o $(BIN_DIR)lexer.o $(BIN_DIR)lextab.o
	$(CC) -o $@ $^ $(CFLAGS) -L $(UTILS_DIR)lib/ -lutils

.PHONY: clean

clean :
	$(RM) *.exe

