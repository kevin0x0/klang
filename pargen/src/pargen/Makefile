INC_DIR = $(PARGEN_DIR)include/
SRC_DIR = $(PARGEN_DIR)src/
BIN_DIR = $(PARGEN_DIR)bin/

#parser path
PS_INC_DIR = $(INC_DIR)parser/
PS_DIR = $(SRC_DIR)parser/
#pargen path
PG_INC_DIR = $(INC_DIR)pargen/
PG_DIR = $(SRC_DIR)pargen/

UTILS_INC = $(UTILS_DIR)include/

all : $(BIN_DIR)cmdline.o $(BIN_DIR)control.o $(BIN_DIR)output.o $(BIN_DIR)convert.o $(BIN_DIR)pg_error.o $(BIN_DIR)dir.o $(BIN_DIR)confhandle.o

$(BIN_DIR)cmdline.o : $(PG_DIR)cmdline.c $(PG_INC_DIR)cmdline.h $(PG_INC_DIR)options.h $(PG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)control.o : $(PG_DIR)control.c $(PG_INC_DIR)control.h $(PG_INC_DIR)options.h $(PS_INC_DIR)parser.h $(PG_INC_DIR)error.h $(UTILS_INC)hashmap/func_map.h $(UTILS_INC)hashmap/strx_map.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)output.o : $(PG_DIR)output.c $(PG_INC_DIR)output.h $(PG_INC_DIR)options.h $(PS_INC_DIR)parser.h $(PG_INC_DIR)error.h $(UTILS_INC)string/kev_string.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)convert.o : $(PG_DIR)convert.c $(PG_INC_DIR)convert.h $(PS_INC_DIR)parser.h $(PG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)pg_error.o : $(PG_DIR)error.c $(PG_INC_DIR)error.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)dir.o : $(PG_DIR)dir.c $(PG_INC_DIR)dir.h $(UTILS_INC)os_spec/dir.h
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_DIR)confhandle.o : $(PG_DIR)confhandle.c $(PG_INC_DIR)confhandle.h $(PG_INC_DIR)error.h $(UTILS_INC)string/kev_string.h $(PS_INC_DIR)parser.h
	$(CC) -c -o $@ $< $(CFLAGS) 
