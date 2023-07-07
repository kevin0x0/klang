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
#tablegen path
TG_INC_PATH	= $(INC_PATH)tablegen/
TG_PATH = $(SRC_PATH)tablegen/
#lexgen path
LG_INC_PATH = $(INC_PATH)lexgen/
LG_PATH = $(SRC_PATH)lexgen/

FA_DEPS = $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h

$(BIN_PATH)liblexgen.a : $(BIN_PATH)cmdline.o $(BIN_PATH)control.o
	$(AR) $@ $^

$(BIN_PATH)cmdline.o : $(LG_PATH)cmdline.c $(LG_INC_PATH)cmdline.h $(LG_INC_PATH)options.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_PATH)control.o : $(LG_PATH)control.c $(LG_INC_PATH)control.h $(LG_INC_PATH)options.h $(PS_INC_PATH)hashmap/strfa_map.h $(PS_INC_PATH)list/pattern_list.h
	$(CC) -c -o $@ $< $(CFLAGS) 

.PHONY: clean

clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a $(BIN_PATH)*.exe
