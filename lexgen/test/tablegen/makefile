OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_PATH = $(abspath ../../../)/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

LEXGEN_PATH = $(ROOT_PATH)lexgen/
UTILS_PATH = $(ROOT_PATH)utils/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automata path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/

RP_INC_PATH = $(INC_PATH)parser/
RP_PATH = $(SRC_PATH)parser/

TD_INC_PATH = $(INC_PATH)tablegen/
TD_PATH = $(SRC_PATH)tablegen/

test.exe : test.c $(BIN_PATH)regex.o $(BIN_PATH)strfa_map.o $(ROOT_PATH)kevfa/lib/libkevfa.a $(UTILS_PATH)lib/libutils.a
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
