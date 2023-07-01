LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automaton path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/
#regex_parser path
RP_INC_PATH = $(INC_PATH)parser/
RP_PATH = $(SRC_PATH)parser/

FA_DEPS = $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h


$(BIN_PATH)libparser.a : $(BIN_PATH)regex.o $(BIN_PATH)strfa_map.o
	$(AR) $(AR_OPTION) $@ $^

$(BIN_PATH)regex.o : $(RP_PATH)regex.c $(RP_INC_PATH)regex.h $(FA_DEPS) $(GEN_INC_PATH)global_def.h $(RP_INC_PATH)hashmap/strfa_map.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

$(BIN_PATH)strfa_map.o : $(RP_PATH)hashmap/strfa_map.c $(RP_INC_PATH)hashmap/strfa_map.h
	$(CC) -c -o $@ $< $(OPTIMIZE) $(CC_GLOBAL_PARAM)

.PHONY: clean

clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a $(BIN_PATH)*.exe
