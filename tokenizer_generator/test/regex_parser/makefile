OPTIMIZE = -g
CC = gcc
AR = ar


ROOT_PATH = C:/Users/flamm/work/kevcc/

CC_GLOBAL_PARAM = -Wall -I $(ROOT_PATH)

TOKENIZER_PATH = $(ROOT_PATH)tokenizer_generator/
INC_PATH = $(TOKENIZER_PATH)include/
SRC_PATH = $(TOKENIZER_PATH)src/
BIN_PATH = $(TOKENIZER_PATH)bin/
LIB_PATH = $(TOKENIZER_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automata path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/

RP_INC_PATH = $(INC_PATH)regex_parser/
RP_PATH = $(SRC_PATH)regex_parser/

test.exe : test.c $(RP_PATH)regex_parser.c $(BIN_PATH)libfa.a
	$(CC) -o $@ $^ $(OPTIMIZE) $(CC_GLOBAL_PARAM)


,PHONY clean:
	rm *.exe
