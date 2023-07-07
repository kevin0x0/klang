LEXGEN_PATH = $(ROOT_PATH)lexgen/
INC_PATH = $(LEXGEN_PATH)include/
SRC_PATH = $(LEXGEN_PATH)src/
BIN_PATH = $(LEXGEN_PATH)bin/
LIB_PATH = $(LEXGEN_PATH)lib/

GEN_INC_PATH = $(INC_PATH)general/

#finite_automaton path
FA_INC_PATH = $(INC_PATH)finite_automaton/
FA_PATH = $(SRC_PATH)finite_automaton/
#tablegen path
TG_INC_PATH = $(INC_PATH)tablegen/
TG_PATH = $(SRC_PATH)tablegen/

FA_DEPS = $(FA_INC_PATH)finite_automaton.h $(FA_INC_PATH)graph.h

$(BIN_PATH)libtablegen.a : $(BIN_PATH)trans_table.o $(BIN_PATH)acc_mapping.o
	$(AR) $@ $^

$(BIN_PATH)trans_table.o : $(TG_PATH)trans_table.c $(TG_INC_PATH)trans_table.h $(FA_DEPS) $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN_PATH)acc_mapping.o : $(TG_PATH)acc_mapping.c $(TG_INC_PATH)acc_mapping.h $(FA_DEPS) $(GEN_INC_PATH)global_def.h
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a $(BIN_PATH)*.exe
