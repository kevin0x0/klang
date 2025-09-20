ROOT_DIR = $(abspath ./)/

INC_DIR = $(ROOT_DIR)include/
SRC_DIR = $(ROOT_DIR)src/
BIN_DIR = $(ROOT_DIR)bin/
LIB_DIR = $(ROOT_DIR)lib/
OBJ_DIR = $(ROOT_DIR)obj/

DEPS_K_DIR = $(ROOT_DIR)deps/k/


AR = ar rcs
CC = gcc
OPTIMIZE = -O3
MEMORY_CHECK =
WARNING = -Wall -Wextra -Winline
DEBUG = -DNDEBUG
PLATFORM = Linux
CONFIG =
STDC =
INSTALL_PATH ?= /usr/local/
CORELIBPATH = $(INSTALL_PATH)/lib/klang/

CFLAGS = $(STDC) $(OPTIMIZE) $(MEMORY_CHECK) $(WARNING) $(DEBUG) $(CONFIG) -I $(ROOT_DIR) -I $(DEPS_K_DIR) -DCORELIBPATH="\"$(CORELIBPATH)\""

KLANG_OBJS = $(OBJ_DIR)klutils.o $(OBJ_DIR)klgc.o $(OBJ_DIR)klmm.o $(OBJ_DIR)kltuple.o $(OBJ_DIR)klarray.o $(OBJ_DIR)klmap.o $(OBJ_DIR)klclass.o \
             $(OBJ_DIR)klclosure.o $(OBJ_DIR)klkfunc.o $(OBJ_DIR)klref.o $(OBJ_DIR)klstring.o $(OBJ_DIR)klcoroutine.o $(OBJ_DIR)klbuiltinclass.o \
             $(OBJ_DIR)klvalue.o $(OBJ_DIR)klcommon.o $(OBJ_DIR)klexec.o $(OBJ_DIR)klstack.o $(OBJ_DIR)klstate.o $(OBJ_DIR)klthrow.o \
             $(OBJ_DIR)klconvert.o $(OBJ_DIR)klapi.o

KLANGC_OBJS = $(OBJ_DIR)klparser.o $(OBJ_DIR)klparser_expr.o $(OBJ_DIR)klparser_stmt.o $(OBJ_DIR)klparser_comprehension.o \
              $(OBJ_DIR)klparser_utils.o $(OBJ_DIR)klparser_error.o $(OBJ_DIR)klparser_recovery.o $(OBJ_DIR)kltokens.o \
              $(OBJ_DIR)kllex.o $(OBJ_DIR)klcfdarr.o $(OBJ_DIR)klerror.o $(OBJ_DIR)klast.o $(OBJ_DIR)klast_expr.o $(OBJ_DIR)klast_stmt.o \
              $(OBJ_DIR)klstrtbl.o $(OBJ_DIR)klcode.o $(OBJ_DIR)klcode_dump.o $(OBJ_DIR)klcode_print.o $(OBJ_DIR)klcontbl.o \
              $(OBJ_DIR)klgen.o $(OBJ_DIR)klgen_expr.o $(OBJ_DIR)klgen_exprbool.o $(OBJ_DIR)klgen_pattern.o $(OBJ_DIR)klgen_stmt.o \
              $(OBJ_DIR)klgen_emit.o $(OBJ_DIR)klgen_utils.o $(OBJ_DIR)klsymtbl.o $(OBJ_DIR)klutils.o


KLANGC_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGC_OBJS))

KLANGLIB_TB_OBJS = $(OBJ_DIR)kllib_tb.o
KLANGLIB_TB_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_TB_OBJS))

KLANGLIB_RTCPL_OBJS = $(OBJ_DIR)kllib_rtcpl.o
KLANGLIB_RTCPL_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_RTCPL_OBJS))

KLANGLIB_BASIC_OBJS = $(OBJ_DIR)kllib_basic.o
KLANGLIB_BASIC_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_BASIC_OBJS))

KLANGLIB_STREAM_OBJS = $(OBJ_DIR)kllib_stream.o $(OBJ_DIR)kllib_file.o $(OBJ_DIR)kllib_stream_collection.o \
                       $(OBJ_DIR)kllib_kistring.o $(OBJ_DIR)kllib_kostring.o $(OBJ_DIR)kllib_sstring.o $(OBJ_DIR)kllib_write.o
KLANGLIB_STREAM_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_STREAM_OBJS))

KLANGLIB_STRING_OBJS = $(OBJ_DIR)kllib_string.o
KLANGLIB_STRING_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_STRING_OBJS))

KLANGLIB_RTCPL_WRAPPER_OBJS = $(OBJ_DIR)kllib_rtcpl_wrapper.o
KLANGLIB_RTCPL_WRAPPER_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_RTCPL_WRAPPER_OBJS))

KLANGLIB_PRINT_OBJS = $(OBJ_DIR)kllib_print.o
KLANGLIB_PRINT_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_PRINT_OBJS))

KLANGLIB_CAST_OBJS = $(OBJ_DIR)kllib_cast.o
KLANGLIB_CAST_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_CAST_OBJS))

KLANGLIB_OS_OBJS = $(OBJ_DIR)kllib_os.o
KLANGLIB_OS_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGLIB_OS_OBJS))


KLANGLIBS_A = $(LIB_DIR)traceback.a $(LIB_DIR)runtime_compiler.a $(LIB_DIR)basic.a $(LIB_DIR)stream.a $(LIB_DIR)string.a $(LIB_DIR)rtcpl_wrapper.a $(LIB_DIR)print.a $(LIB_DIR)cast.a $(LIB_DIR)os.a
KLANGLIBS_SO = $(patsubst %.a, %.so, $(KLANGLIBS_A))


ifeq ($(PLATFORM), Windows)

all: $(LIB_DIR)klang.dll $(LIB_DIR)libklangc.a $(LIB_DIR)libklangc.pic.a $(BIN_DIR)klangc $(BIN_DIR)klang $(KLANGLIBS_SO)

$(BIN_DIR)klang : $(OBJ_DIR)klang.o $(LIB_DIR)klang.dll $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) $(CFLAGS) -o $@ $^

$(LIB_DIR)klang.dll : $(KLANG_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared -o $@ $^


$(LIB_DIR)traceback.so : $(KLANGLIB_TB_PIC_OBJS) $(LIB_DIR)klang.dll $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)runtime_compiler.so : $(KLANGLIB_RTCPL_PIC_OBJS) $(LIB_DIR)klang.dll $(LIB_DIR)libklangc.pic.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)basic.so : $(KLANGLIB_BASIC_PIC_OBJS) $(LIB_DIR)klang.dll $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)stream.so : $(KLANGLIB_STREAM_PIC_OBJS) $(LIB_DIR)klang.dll $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)string.so : $(KLANGLIB_STRING_OBJS) $(LIB_DIR)klang.dll | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)rtcpl_wrapper.so : $(KLANGLIB_RTCPL_WRAPPER_PIC_OBJS) $(LIB_DIR)klang.dll $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)print.so : $(KLANGLIB_PRINT_PIC_OBJS) $(LIB_DIR)klang.dll | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)cast.so : $(KLANGLIB_CAST_PIC_OBJS) $(LIB_DIR)klang.dll | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)os.so : $(KLANGLIB_OS_PIC_OBJS) $(LIB_DIR)klang.dll | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

endif

ifeq ($(PLATFORM), Linux)

all: $(LIB_DIR)libklang.a $(LIB_DIR)libklangc.a $(LIB_DIR)libklangc.pic.a $(BIN_DIR)klangc $(BIN_DIR)klang $(KLANGLIBS_SO)

$(BIN_DIR)klang : $(OBJ_DIR)klang.o $(LIB_DIR)libklang.a | create_dir
	$(CC) $(CFLAGS) -rdynamic -o $@ $< -L $(LIB_DIR) -lklang -ldl

$(LIB_DIR)libklang.a : $(KLANG_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.a $@
	$(AR) $@ $(KLANG_OBJS)


$(LIB_DIR)traceback.so : $(KLANGLIB_TB_PIC_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $(KLANGLIB_TB_PIC_OBJS)

$(LIB_DIR)runtime_compiler.so : $(KLANGLIB_RTCPL_PIC_OBJS) $(LIB_DIR)libklangc.pic.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $(KLANGLIB_RTCPL_PIC_OBJS) -L $(LIB_DIR) -lklangc.pic

$(LIB_DIR)basic.so : $(KLANGLIB_BASIC_PIC_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $(KLANGLIB_BASIC_PIC_OBJS)

$(LIB_DIR)stream.so : $(KLANGLIB_STREAM_PIC_OBJS) $(DEPS_K_DIR)lib/libk.pic.a | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)string.so : $(KLANGLIB_STRING_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)rtcpl_wrapper.so : $(KLANGLIB_RTCPL_WRAPPER_PIC_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)print.so : $(KLANGLIB_PRINT_PIC_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)cast.so : $(KLANGLIB_CAST_PIC_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

$(LIB_DIR)os.so : $(KLANGLIB_OS_PIC_OBJS) | create_dir
	$(CC) -shared $(CFLAGS) -o $@ $^

endif

ifeq ($(PLATFORM), static)

CONFIG += -DKLCONFIG_USE_STATIC_LANGLIB --static


all: $(LIB_DIR)libklang.a $(LIB_DIR)libklangc.a $(BIN_DIR)klangc $(BIN_DIR)klang

$(BIN_DIR)klang : $(OBJ_DIR)klang.o $(LIB_DIR)libklang.a $(KLANGLIBS_A) | create_dir
	$(CC) $(CFLAGS) -o $@ $^

$(LIB_DIR)libklang.a : $(KLANG_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.a $@
	$(AR) $@ $(KLANG_OBJS)


$(LIB_DIR)traceback.a : $(KLANGLIB_TB_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)runtime_compiler.a : $(KLANGLIB_RTCPL_OBJS) $(LIB_DIR)libklangc.a | create_dir
	cp $(LIB_DIR)libklangc.a $@
	$(AR) $@ $(KLANGLIB_RTCPL_OBJS)

$(LIB_DIR)basic.a : $(KLANGLIB_BASIC_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)stream.a : $(KLANGLIB_STREAM_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	$(AR) $@ $^

$(LIB_DIR)string.a : $(KLANGLIB_STRING_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)rtcpl_wrapper.a : $(KLANGLIB_RTCPL_WRAPPER_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)print.a : $(KLANGLIB_PRINT_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)cast.a : $(KLANGLIB_CAST_OBJS) | create_dir
	$(AR) $@ $^

$(LIB_DIR)os.a : $(KLANGLIB_OS_OBJS) | create_dir
	$(AR) $@ $^

endif

install: all
	mkdir -p $(INSTALL_PATH)/bin $(CORELIBPATH)
	install $(BIN_DIR)/klang $(BIN_DIR)/klangc $(INSTALL_PATH)/bin
	install $(LIB_DIR)/* $(CORELIBPATH)


$(BIN_DIR)klangc : $(OBJ_DIR)klangc.o $(LIB_DIR)libklangc.a | create_dir
	$(CC) $(CFLAGS) -o $@ $< -L $(LIB_DIR) -lklangc

$(LIB_DIR)libklangc.a : $(KLANGC_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.a $@
	$(AR) $@ $(KLANGC_OBJS)

$(LIB_DIR)libklangc.pic.a : $(KLANGC_PIC_OBJS) $(DEPS_K_DIR)lib/libk.pic.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.pic.a $@
	$(AR) $@ $(KLANGC_PIC_OBJS)



$(DEPS_K_DIR)lib/libk.a :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.a "STDC=$(STDC)" "AR=$(AR)" "CC=$(CC)" "OPTIMIZE=$(OPTIMIZE)" "MEMORY_CHECK=$(MEMORY_CHECK)" "WARNING=$(WARNING)" "DEBUG=$(DEBUG)"

$(DEPS_K_DIR)lib/libk.pic.a :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.pic.a "STDC=$(STDC)" "AR=$(AR)" "CC=$(CC)" "OPTIMIZE=$(OPTIMIZE)" "MEMORY_CHECK=$(MEMORY_CHECK)" "WARNING=$(WARNING)" "DEBUG=$(DEBUG)"

$(DEPS_K_DIR)lib/libk.so :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.so "STDC=$(STDC)" "AR=$(AR)" "CC=$(CC)" "OPTIMIZE=$(OPTIMIZE)" "MEMORY_CHECK=$(MEMORY_CHECK)" "WARNING=$(WARNING)" "DEBUG=$(DEBUG)"





$(OBJ_DIR)klast.o : $(SRC_DIR)ast/klast.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klast_expr.o : $(SRC_DIR)ast/klast_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klast_stmt.o : $(SRC_DIR)ast/klast_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klstrtbl.o : $(SRC_DIR)ast/klstrtbl.c $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcode.o : $(SRC_DIR)code/klcode.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcode_dump.o : $(SRC_DIR)code/klcode_dump.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcode_print.o : $(SRC_DIR)code/klcode_print.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcontbl.o : $(SRC_DIR)code/klcontbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen.o : $(SRC_DIR)code/klgen.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_emit.o : $(SRC_DIR)code/klgen_emit.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_expr.o : $(SRC_DIR)code/klgen_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_exprbool.o : $(SRC_DIR)code/klgen_exprbool.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_pattern.o : $(SRC_DIR)code/klgen_pattern.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_stmt.o : $(SRC_DIR)code/klgen_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_pattern.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_utils.o : $(SRC_DIR)code/klgen_utils.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_emit.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klsymtbl.o : $(SRC_DIR)code/klsymtbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klerror.o : $(SRC_DIR)error/klerror.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(INC_DIR)error/klerror.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klapi.o : $(SRC_DIR)klapi.c $(DEPS_K_DIR)include/lib/lib.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h $(INC_DIR)lang/klconvert.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klconvert.o : $(SRC_DIR)lang/klconvert.c $(INC_DIR)lang/klconvert.h $(INC_DIR)lang/kltypes.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_basic.o : $(SRC_DIR)langlib/basic/kllib_basic.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_cast.o : $(SRC_DIR)langlib/cast/kllib_cast.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_math.o : $(SRC_DIR)langlib/math/kllib_math.c | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_os.o : $(SRC_DIR)langlib/os/kllib_os.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_print.o : $(SRC_DIR)langlib/print/kllib_print.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_rtcpl.o : $(SRC_DIR)langlib/rtcpl/kllib_rtcpl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)error/klerror.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/kllex.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_rtcpl_wrapper.o : $(SRC_DIR)langlib/rtcpl_wrapper/kllib_rtcpl_wrapper.c $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kibuf.h $(DEPS_K_DIR)include/os_spec/kfs.h $(DEPS_K_DIR)include/string/kstring.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_file.o : $(SRC_DIR)langlib/stream/kllib_file.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)langlib/stream/kllib_file.h $(INC_DIR)langlib/stream/kllib_stream.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_kistring.o : $(SRC_DIR)langlib/stream/kllib_kistring.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(INC_DIR)langlib/stream/kllib_kistring.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_kostring.o : $(SRC_DIR)langlib/stream/kllib_kostring.c $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)langlib/stream/kllib_kostring.h $(INC_DIR)langlib/stream/kllib_strbuf.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_sstring.o : $(SRC_DIR)langlib/stream/kllib_sstring.c $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)langlib/stream/kllib_kistring.h $(INC_DIR)langlib/stream/kllib_kostring.h $(INC_DIR)langlib/stream/kllib_strbuf.h $(INC_DIR)langlib/stream/kllib_stream.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_stream.o : $(SRC_DIR)langlib/stream/kllib_stream.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)langlib/stream/kllib_stream.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)langlib/stream/kllib_strbuf.h $(INC_DIR)langlib/stream/kllib_write.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_stream_collection.o : $(SRC_DIR)langlib/stream/kllib_stream_collection.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h $(INC_DIR)langlib/stream/kllib_stream.h $(INC_DIR)langlib/stream/kllib_sstring.h $(INC_DIR)langlib/stream/kllib_file.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_write.o : $(SRC_DIR)langlib/stream/kllib_write.c $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ki.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)langlib/stream/kllib_write.h $(INC_DIR)langlib/stream/kllib_stream.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_string.o : $(SRC_DIR)langlib/string/kllib_string.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_tb.o : $(SRC_DIR)langlib/traceback/kllib_tb.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klang.o : $(SRC_DIR)main/klang.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/kibuf.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/os_spec/kfs.h $(DEPS_K_DIR)include/string/kstring.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klangc.o : $(SRC_DIR)main/klangc.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/os_spec/kfs.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klutils.o : $(SRC_DIR)misc/klutils.c $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgc.o : $(SRC_DIR)mm/klgc.c $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klmm.o : $(SRC_DIR)mm/klmm.c $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcfdarr.o : $(SRC_DIR)parse/klcfdarr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klcfdarr.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllex.o : $(SRC_DIR)parse/kllex.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser.o : $(SRC_DIR)parse/klparser.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/klparser_stmt.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_comprehension.o : $(SRC_DIR)parse/klparser_comprehension.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser_comprehension.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/klparser_stmt.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_error.o : $(SRC_DIR)parse/klparser_error.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_expr.o : $(SRC_DIR)parse/klparser_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/klparser_stmt.h $(INC_DIR)parse/klparser_comprehension.h $(INC_DIR)parse/klcfdarr.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_recovery.o : $(SRC_DIR)parse/klparser_recovery.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_stmt.o : $(SRC_DIR)parse/klparser_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser_stmt.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_recovery.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_utils.o : $(SRC_DIR)parse/klparser_utils.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kltokens.o : $(SRC_DIR)parse/kltokens.c $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klarray.o : $(SRC_DIR)value/klarray.c $(INC_DIR)value/klarray.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klbuiltinclass.o : $(SRC_DIR)value/klbuiltinclass.c $(INC_DIR)value/klbuiltinclass.h $(INC_DIR)value/klclass.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)mm/klmm.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klarray.h $(INC_DIR)value/klstate.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klclass.o : $(SRC_DIR)value/klclass.c $(INC_DIR)value/klclass.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)mm/klmm.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klclosure.o : $(SRC_DIR)value/klclosure.c $(INC_DIR)value/klclosure.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klkfunc.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcoroutine.o : $(SRC_DIR)value/klcoroutine.c $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klkfunc.o : $(SRC_DIR)value/klkfunc.c $(INC_DIR)value/klkfunc.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klmap.o : $(SRC_DIR)value/klmap.c $(INC_DIR)value/klmap.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klstring.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klref.o : $(SRC_DIR)value/klref.c $(INC_DIR)value/klref.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klstate.o : $(SRC_DIR)value/klstate.c $(INC_DIR)value/klstate.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klstring.o : $(SRC_DIR)value/klstring.c $(INC_DIR)value/klstring.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kltuple.o : $(SRC_DIR)value/kltuple.c $(INC_DIR)value/kltuple.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klvalue.o : $(SRC_DIR)value/klvalue.c $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcommon.o : $(SRC_DIR)vm/klcommon.c $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)mm/klmm.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klbuiltinclass.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klexec.o : $(SRC_DIR)vm/klexec.c $(INC_DIR)vm/klexec.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconvert.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klstack.o : $(SRC_DIR)vm/klstack.c $(INC_DIR)vm/klstack.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klthrow.o : $(SRC_DIR)vm/klthrow.c $(INC_DIR)vm/klthrow.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) $(CFLAGS) -c -o $@ $<




# ==============================SHARED OBJECT==================================
$(OBJ_DIR)klast.pic.o : $(SRC_DIR)ast/klast.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klast_expr.pic.o : $(SRC_DIR)ast/klast_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klast_stmt.pic.o : $(SRC_DIR)ast/klast_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klstrtbl.pic.o : $(SRC_DIR)ast/klstrtbl.c $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcode.pic.o : $(SRC_DIR)code/klcode.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcode_dump.pic.o : $(SRC_DIR)code/klcode_dump.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcode_print.pic.o : $(SRC_DIR)code/klcode_print.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcontbl.pic.o : $(SRC_DIR)code/klcontbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen.pic.o : $(SRC_DIR)code/klgen.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_emit.pic.o : $(SRC_DIR)code/klgen_emit.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_expr.pic.o : $(SRC_DIR)code/klgen_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_exprbool.pic.o : $(SRC_DIR)code/klgen_exprbool.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_pattern.pic.o : $(SRC_DIR)code/klgen_pattern.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_stmt.pic.o : $(SRC_DIR)code/klgen_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen_emit.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_pattern.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klgen_utils.pic.o : $(SRC_DIR)code/klgen_utils.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_utils.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_emit.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klsymtbl.pic.o : $(SRC_DIR)code/klsymtbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klerror.pic.o : $(SRC_DIR)error/klerror.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(INC_DIR)error/klerror.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klutils.pic.o : $(SRC_DIR)misc/klutils.c $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klcfdarr.pic.o : $(SRC_DIR)parse/klcfdarr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klcfdarr.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllex.pic.o : $(SRC_DIR)parse/kllex.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser.pic.o : $(SRC_DIR)parse/klparser.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/klparser_stmt.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_comprehension.pic.o : $(SRC_DIR)parse/klparser_comprehension.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser_comprehension.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/klparser_stmt.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_error.pic.o : $(SRC_DIR)parse/klparser_error.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_expr.pic.o : $(SRC_DIR)parse/klparser_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/klparser_stmt.h $(INC_DIR)parse/klparser_comprehension.h $(INC_DIR)parse/klcfdarr.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_recovery.pic.o : $(SRC_DIR)parse/klparser_recovery.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)parse/klparser_recovery.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_stmt.pic.o : $(SRC_DIR)parse/klparser_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser_stmt.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_expr.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser_recovery.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)klparser_utils.pic.o : $(SRC_DIR)parse/klparser_utils.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klparser_utils.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klparser_error.h $(INC_DIR)parse/klparser_recovery.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kltokens.pic.o : $(SRC_DIR)parse/kltokens.c $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_basic.pic.o : $(SRC_DIR)langlib/basic/kllib_basic.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_cast.pic.o : $(SRC_DIR)langlib/cast/kllib_cast.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_math.pic.o : $(SRC_DIR)langlib/math/kllib_math.c | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_os.pic.o : $(SRC_DIR)langlib/os/kllib_os.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_print.pic.o : $(SRC_DIR)langlib/print/kllib_print.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_rtcpl.pic.o : $(SRC_DIR)langlib/rtcpl/kllib_rtcpl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h $(INC_DIR)code/klcode.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)error/klerror.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/klparser.h $(INC_DIR)parse/kllex.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_rtcpl_wrapper.pic.o : $(SRC_DIR)langlib/rtcpl_wrapper/kllib_rtcpl_wrapper.c $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kibuf.h $(DEPS_K_DIR)include/os_spec/kfs.h $(DEPS_K_DIR)include/string/kstring.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_file.pic.o : $(SRC_DIR)langlib/stream/kllib_file.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)langlib/stream/kllib_file.h $(INC_DIR)langlib/stream/kllib_stream.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_kistring.pic.o : $(SRC_DIR)langlib/stream/kllib_kistring.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(INC_DIR)langlib/stream/kllib_kistring.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_kostring.pic.o : $(SRC_DIR)langlib/stream/kllib_kostring.c $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)langlib/stream/kllib_kostring.h $(INC_DIR)langlib/stream/kllib_strbuf.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_sstring.pic.o : $(SRC_DIR)langlib/stream/kllib_sstring.c $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)langlib/stream/kllib_kistring.h $(INC_DIR)langlib/stream/kllib_kostring.h $(INC_DIR)langlib/stream/kllib_strbuf.h $(INC_DIR)langlib/stream/kllib_stream.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_stream.pic.o : $(SRC_DIR)langlib/stream/kllib_stream.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)langlib/stream/kllib_stream.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)langlib/stream/kllib_strbuf.h $(INC_DIR)langlib/stream/kllib_write.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_stream_collection.pic.o : $(SRC_DIR)langlib/stream/kllib_stream_collection.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h $(INC_DIR)langlib/stream/kllib_stream.h $(INC_DIR)langlib/stream/kllib_sstring.h $(INC_DIR)langlib/stream/kllib_file.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_write.pic.o : $(SRC_DIR)langlib/stream/kllib_write.c $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ki.h $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)langlib/stream/kllib_write.h $(INC_DIR)langlib/stream/kllib_stream.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_string.pic.o : $(SRC_DIR)langlib/string/kllib_string.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)kllib_tb.pic.o : $(SRC_DIR)langlib/traceback/kllib_tb.c $(INC_DIR)klapi.h $(INC_DIR)vm/klexception.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/kltuple.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconfig.h | create_dir
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<





.PHONY: clean create_dir all
clean :
	$(RM) -r $(OBJ_DIR)* $(LIB_DIR)* $(BIN_DIR)*
	$(MAKE) -C $(DEPS_K_DIR) clean

create_dir :
	mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)
