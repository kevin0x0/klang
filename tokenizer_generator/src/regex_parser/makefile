TOKENIZER_PATH = $(ROOT_PATH)tokenizer_generator/
INC_PATH = $(TOKENIZER_PATH)include/
SRC_PATH = $(TOKENIZER_PATH)src/
BIN_PATH = $(TOKENIZER_PATH)bin/
LIB_PATH = $(TOKENIZER_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automaton path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/
#regex_parser path
RP_INC_PATH = $(INC_PATH)regex_parser/
RP_PATH = $(SRC_PATH)regex_parser/

FA_DEPS = $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h


$(BIN_PATH)libregex_parser.a : $(BIN_PATH)regex_parser.o $(BIN_PATH)strfa_map.o
	$(AR) $(AR_OPTION) $@ $^

$(BIN_PATH)regex_parser.o : $(RP_PATH)regex_parser.c $(RP_INC_PATH)regex_parser.h $(FA_DEPS) $(GEN_INC_PATH)global_def.h $(RP_INC_PATH)hahmap/strfa_map.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)strfa_map.o : $(RP_PATH)hashmap/strfa_map.c $(RP_INC_PATH)hashmap/strfa_map.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

.PHONY: clean

clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a $(BIN_PATH)*.exe
