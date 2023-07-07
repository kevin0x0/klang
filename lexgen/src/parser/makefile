LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automaton path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/
#parser path
PS_INC_PATH = $(INC_PATH)parser/
PS_PATH = $(SRC_PATH)parser/

FA_DEPS = $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h


$(BIN_PATH)libparser.a : $(BIN_PATH)regex.o $(BIN_PATH)parser.o $(BIN_PATH)lexer.o $(BIN_PATH)error.o $(BIN_PATH)lexgenlexer_table.o $(BIN_PATH)strfa_map.o $(BIN_PATH)pattern_list.o
	$(AR) $@ $^

$(BIN_PATH)regex.o : $(PS_PATH)regex.c $(PS_INC_PATH)regex.h $(FA_DEPS) $(GEN_INC_PATH)global_def.h $(PS_INC_PATH)hashmap/strfa_map.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)parser.o : $(PS_PATH)parser.c $(PS_INC_PATH)parser.h $(PS_INC_PATH)error.h $(PS_INC_PATH)lexer.h $(PS_INC_PATH)regex.h $(FA_DEPS) $(GEN_INC_PATH)global_def.h $(PS_INC_PATH)hashmap/strfa_map.h $(PS_INC_PATH)list/pattern_list.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)lexer.o : $(PS_PATH)lexer.c $(PS_INC_PATH)lexer.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)error.o : $(PS_PATH)error.c $(PS_INC_PATH)error.h $(PS_INC_PATH)lexer.h $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)lexgenlexer_table.o : $(PS_PATH)lexgenlexer_table.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)strfa_map.o : $(PS_PATH)hashmap/strfa_map.c $(PS_INC_PATH)hashmap/strfa_map.h $(FA_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)pattern_list.o : $(PS_PATH)list/pattern_list.c $(PS_INC_PATH)list/pattern_list.h  $(FA_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a $(BIN_PATH)*.exe
