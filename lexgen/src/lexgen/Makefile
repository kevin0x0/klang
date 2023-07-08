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
#tablegen path
TG_INC_PATH	= $(INC_PATH)tablegen/
TG_PATH = $(SRC_PATH)tablegen/
#lexgen path
LG_INC_PATH = $(INC_PATH)lexgen/
LG_PATH = $(SRC_PATH)lexgen/

FA_DEPS = $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h

$(BIN_PATH)liblexgen.a : $(BIN_PATH)cmdline.o $(BIN_PATH)control.o $(BIN_PATH)template.o $(BIN_PATH)output.o
	$(AR) $@ $^

$(BIN_PATH)cmdline.o : $(LG_PATH)cmdline.c $(LG_INC_PATH)cmdline.h $(LG_INC_PATH)options.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_PATH)control.o : $(LG_PATH)control.c $(LG_INC_PATH)control.h $(LG_INC_PATH)options.h $(PS_INC_PATH)parser.h $(PS_INC_PATH)hashmap/strfa_map.h $(PS_INC_PATH)list/pattern_list.h $(FA_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_PATH)output.o : $(LG_PATH)output.c $(LG_INC_PATH)output.h $(LG_INC_PATH)options.h $(PS_INC_PATH)hashmap/strfa_map.h $(PS_INC_PATH)parser.h $(PS_INC_PATH)hashmap/strfa_map.h $(PS_INC_PATH)list/pattern_list.h $(LG_INC_PATH)template.h $(FA_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_PATH)template.o : $(LG_PATH)template.c $(LG_INC_PATH)template.h
	$(CC) -c -o $@ $< $(CFLAGS) 
