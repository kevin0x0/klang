UTILS_PATH = $(ROOT_PATH)utils/
INC_PATH = $(UTILS_PATH)include/
SRC_PATH = $(UTILS_PATH)src/
BIN_PATH = $(UTILS_PATH)bin/
LIB_PATH = $(UTILS_PATH)lib/

$(BIN_PATH)utils.a : $(BIN_PATH)dir.o $(BIN_PATH)exec.o
	mkdir -p $(BIN_PATH)
	$(AR) $@ $^

$(BIN_PATH)dir.o : $(SRC_PATH)dir.c $(INC_PATH)dir.h
	mkdir -p $(BIN_PATH)
	$(CC) -c -o $@ $< $(CFLAGS) 

$(BIN_PATH)exec.o : $(SRC_PATH)exec.c $(INC_PATH)exec.h
	mkdir -p $(BIN_PATH)
	$(CC) -c -o $@ $< $(CFLAGS) 

.PHONY: clean
clean :
	rm $(BIN_PATH)*.o $(BIN_PATH)*.a
