OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_PATH = $(abspath ../../../)/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automata path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/

test.exe : test.c $(BIN_PATH)libparser.a $(BIN_PATH)libfa.a
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
