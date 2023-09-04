INC_DIR = $(LEXGEN_DIR)include/
SRC_DIR = $(LEXGEN_DIR)src/
BIN_DIR = $(LEXGEN_DIR)bin/
LIB_DIR = $(LEXGEN_DIR)lib/

#finite_automaton path
FA_INC_DIR = $(INC_DIR)finite_automaton/
FA_DIR = $(SRC_DIR)finite_automaton/
#parser path
PS_INC_DIR = $(INC_DIR)parser/
PS_DIR = $(SRC_DIR)parser/

FA_DEPS = $(FA_INC_DIR)finite_automaton.h $(FA_INC_DIR)graph.h

UTILS_INC = $(UTILS_DIR)include/
GEN_INC_DIR = $(UTILS_INC)general/

$(BIN_DIR)libparser.a : $(BIN_DIR)regex.o $(BIN_DIR)parser.o $(BIN_DIR)lexer.o $(BIN_DIR)par_error.o $(BIN_DIR)lexgenlexer_table.o $(BIN_DIR)strfa_map.o $(BIN_DIR)pattern_list.o
	$(AR) $@ $^

$(BIN_DIR)regex.o : $(PS_DIR)regex.c $(PS_INC_DIR)regex.h $(FA_DEPS) $(GEN_INC_DIR)global_def.h $(PS_INC_DIR)hashmap/strfa_map.h $(UTILS_INC)string/kev_string.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)parser.o : $(PS_DIR)parser.c $(PS_INC_DIR)parser.h $(PS_INC_DIR)error.h $(PS_INC_DIR)lexer.h $(PS_INC_DIR)regex.h $(FA_DEPS) $(GEN_INC_DIR)global_def.h $(UTILS_INC)hashmap/str_map.h $(PS_INC_DIR)hashmap/strfa_map.h $(PS_INC_DIR)list/pattern_list.h $(UTILS_INC)string/kev_string.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lexer.o : $(PS_DIR)lexer.c $(PS_INC_DIR)lexer.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)par_error.o : $(PS_DIR)error.c $(PS_INC_DIR)error.h $(PS_INC_DIR)lexer.h $(GEN_INC_DIR)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)lexgenlexer_table.o : $(PS_DIR)lexgenlexer_table.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)strfa_map.o : $(PS_DIR)hashmap/strfa_map.c $(PS_INC_DIR)hashmap/strfa_map.h $(FA_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_DIR)pattern_list.o : $(PS_DIR)list/pattern_list.c $(PS_INC_DIR)list/pattern_list.h  $(FA_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
