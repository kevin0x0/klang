ROOT_DIR = $(abspath ./)/

INC_DIR = $(ROOT_DIR)include/
SRC_DIR = $(ROOT_DIR)src/
LIB_DIR = $(ROOT_DIR)lib/
OBJ_DIR = $(ROOT_DIR)obj/

DEPS_K_DIR = $(ROOT_DIR)deps/k/


AR = ar rcs
CC = gcc
OPTIMIZE = -O2
MEMORY_CHECK =
WARNING = -Wall -Wextra -Winline
DEBUG = -DNDEBUG

CFLAGS = $(OPTIMIZE) $(MEMORY_CHECK) $(WARNING) $(DEBUG) -I $(ROOT_DIR) -I $(DEPS_K_DIR)

KLANG_OBJS = $(OBJ_DIR)klapi.o $(OBJ_DIR)klutils.o $(OBJ_DIR)klgc.o $(OBJ_DIR)klmm.o $(OBJ_DIR)klarray.o $(OBJ_DIR)klmap.o \
             $(OBJ_DIR)klclass.o $(OBJ_DIR)klclosure.o $(OBJ_DIR)klkfunc.o $(OBJ_DIR)klref.o $(OBJ_DIR)klstring.o $(OBJ_DIR)klcoroutine.o \
             $(OBJ_DIR)klvalue.o $(OBJ_DIR)klcommon.o $(OBJ_DIR)klexec.o $(OBJ_DIR)klstack.o $(OBJ_DIR)klstate.o $(OBJ_DIR)klthrow.o \
             $(OBJ_DIR)klconvert.o

KLANGC_OBJS = $(OBJ_DIR)klparser.o $(OBJ_DIR)kltokens.o $(OBJ_DIR)kllex.o $(OBJ_DIR)klidarr.o $(OBJ_DIR)klcfdarr.o \
              $(OBJ_DIR)klerror.o $(OBJ_DIR)klast_expr.o $(OBJ_DIR)klast_stmt.o $(OBJ_DIR)klstrtbl.o $(OBJ_DIR)klcode.o \
              $(OBJ_DIR)klcode_dump.o $(OBJ_DIR)klcode_print.o $(OBJ_DIR)klcontbl.o $(OBJ_DIR)klgen.o $(OBJ_DIR)klgen_expr.o \
              $(OBJ_DIR)klgen_exprbool.o $(OBJ_DIR)klgen_pattern.o $(OBJ_DIR)klgen_stmt.o $(OBJ_DIR)klsymtbl.o

KLANGC_PIC_OBJS = $(patsubst %.o, %.pic.o, $(KLANGC_OBJS))


all: $(LIB_DIR)libklang.a $(LIB_DIR)libklangc.a $(LIB_DIR)libklangc.so 


$(LIB_DIR)libklang.a : $(KLANG_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.a $(OBJ_DIR)libklang.a
	$(AR) $@ $(KLANG_OBJS)

$(LIB_DIR)libklangc.a : $(KLANGC_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	cp $(DEPS_K_DIR)lib/libk.a $(OBJ_DIR)libklangc.a
	$(AR) $@ $(KLANGC_OBJS)

$(LIB_DIR)libklangc.so : $(KLANGC_PIC_OBJS) $(DEPS_K_DIR)lib/libk.a | create_dir
	$(CC) -shared -o $@ $(KLANGC_PIC_OBJS) -L $(DEPS_K_DIR)lib/ -Wl,-Bstatic -lk -Wl,-Bdynamic


$(DEPS_K_DIR)lib/libk.a :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.a "AR=$(AR)" "CC=$(CC)" "OPTIMIZE=$(OPTIMIZE)" "MEMORY_CHECK=$(MEMORY_CHECK)" "WARNING=$(WARNING)" "DEBUG=$(DEBUG)"

$(DEPS_K_DIR)lib/libk.so :
	$(MAKE) -C $(DEPS_K_DIR) $(DEPS_K_DIR)lib/libk.so "AR=$(AR)" "CC=$(CC)" "OPTIMIZE=$(OPTIMIZE)" "MEMORY_CHECK=$(MEMORY_CHECK)" "WARNING=$(WARNING)" "DEBUG=$(DEBUG)"





$(OBJ_DIR)klast_expr.o : $(SRC_DIR)ast/klast_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klast_stmt.o : $(SRC_DIR)ast/klast_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstrtbl.o : $(SRC_DIR)ast/klstrtbl.c $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcode.o : $(SRC_DIR)code/klcode.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klsymtbl.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcode_dump.o : $(SRC_DIR)code/klcode_dump.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcode_print.o : $(SRC_DIR)code/klcode_print.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcontbl.o : $(SRC_DIR)code/klcontbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen.o : $(SRC_DIR)code/klgen.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen_expr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_exprbool.o : $(SRC_DIR)code/klgen_exprbool.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_expr.o : $(SRC_DIR)code/klgen_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_pattern.o : $(SRC_DIR)code/klgen_pattern.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen_expr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgen_stmt.o : $(SRC_DIR)code/klgen_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_pattern.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klsymtbl.o : $(SRC_DIR)code/klsymtbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klerror.o : $(SRC_DIR)error/klerror.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)error/klerror.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klapi.o : $(SRC_DIR)klapi.c $(DEPS_K_DIR)include/lib/lib.h $(INC_DIR)klapi.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klvalue.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)vm/klexec.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconvert.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klconvert.o : $(SRC_DIR)lang/klconvert.c $(INC_DIR)lang/klconvert.h $(INC_DIR)lang/kltypes.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klangc.o : $(SRC_DIR)main/klangc.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/kio/kifile.h $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kofile.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/kio/kibuf.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klutils.o : $(SRC_DIR)misc/klutils.c $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klgc.o : $(SRC_DIR)mm/klgc.c $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klmm.o : $(SRC_DIR)mm/klmm.c $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcfdarr.o : $(SRC_DIR)parse/klcfdarr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klcfdarr.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klidarr.o : $(SRC_DIR)parse/klidarr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klidarr.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)kllex.o : $(SRC_DIR)parse/kllex.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klparser.o : $(SRC_DIR)parse/klparser.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/karray.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klcfdarr.h $(INC_DIR)parse/klidarr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)kltokens.o : $(SRC_DIR)parse/kltokens.c $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klarray.o : $(SRC_DIR)value/klarray.c $(INC_DIR)value/klarray.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klclass.o : $(SRC_DIR)value/klclass.c $(INC_DIR)value/klclass.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)mm/klmm.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klclosure.o : $(SRC_DIR)value/klclosure.c $(INC_DIR)value/klclosure.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klkfunc.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klmap.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcoroutine.o : $(SRC_DIR)value/klcoroutine.c $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)vm/klexec.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klkfunc.o : $(SRC_DIR)value/klkfunc.c $(INC_DIR)value/klkfunc.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klmap.o : $(SRC_DIR)value/klmap.c $(INC_DIR)value/klmap.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klref.o : $(SRC_DIR)value/klref.c $(INC_DIR)value/klref.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstate.o : $(SRC_DIR)value/klstate.c $(INC_DIR)value/klstate.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstring.o : $(SRC_DIR)value/klstring.c $(INC_DIR)value/klstring.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klvalue.o : $(SRC_DIR)value/klvalue.c $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klcommon.o : $(SRC_DIR)vm/klcommon.c $(INC_DIR)vm/klcommon.h $(INC_DIR)value/klclass.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klstring.h $(INC_DIR)mm/klmm.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klmap.h $(INC_DIR)value/klarray.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klexec.o : $(SRC_DIR)vm/klexec.c $(INC_DIR)vm/klexec.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klcfunc.h $(INC_DIR)value/klcoroutine.h $(INC_DIR)misc/klutils.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)value/klclosure.h $(INC_DIR)value/klkfunc.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klref.h $(INC_DIR)lang/klinst.h $(INC_DIR)value/klstate.h $(INC_DIR)vm/klstack.h $(INC_DIR)value/klmap.h $(INC_DIR)value/klclass.h $(INC_DIR)value/klstring.h $(INC_DIR)vm/klcommon.h $(INC_DIR)vm/klthrow.h $(INC_DIR)value/klarray.h $(INC_DIR)lang/klconvert.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klstack.o : $(SRC_DIR)vm/klstack.c $(INC_DIR)vm/klstack.h $(INC_DIR)mm/klmm.h $(INC_DIR)lang/kltypes.h $(INC_DIR)misc/klutils.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)vm/klexception.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ_DIR)klthrow.o : $(SRC_DIR)vm/klthrow.c $(INC_DIR)vm/klthrow.h $(INC_DIR)vm/klexception.h $(INC_DIR)value/klvalue.h $(INC_DIR)value/klcfunc.h $(INC_DIR)lang/kltypes.h $(INC_DIR)mm/klmm.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS)




# ==============================SHARED OBJECT==================================
$(OBJ_DIR)klast_expr.pic.o : $(SRC_DIR)ast/klast_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klast_stmt.pic.o : $(SRC_DIR)ast/klast_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klstrtbl.pic.o : $(SRC_DIR)ast/klstrtbl.c $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcode.pic.o : $(SRC_DIR)code/klcode.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klsymtbl.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcode_dump.pic.o : $(SRC_DIR)code/klcode_dump.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcode_print.pic.o : $(SRC_DIR)code/klcode_print.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcontbl.pic.o : $(SRC_DIR)code/klcontbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen.pic.o : $(SRC_DIR)code/klgen.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen_expr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_exprbool.pic.o : $(SRC_DIR)code/klgen_exprbool.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_expr.pic.o : $(SRC_DIR)code/klgen_expr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_pattern.pic.o : $(SRC_DIR)code/klgen_pattern.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_pattern.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen_expr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klgen_stmt.pic.o : $(SRC_DIR)code/klgen_stmt.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/kgarray.h $(INC_DIR)code/klgen_stmt.h $(INC_DIR)code/klgen.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcodeval.h $(INC_DIR)lang/klinst.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)code/klgen_expr.h $(INC_DIR)code/klgen_exprbool.h $(INC_DIR)code/klgen_pattern.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klsymtbl.pic.o : $(SRC_DIR)code/klsymtbl.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/karray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)code/klsymtbl.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)code/klcodeval.h $(INC_DIR)ast/klast.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)code/klcode.h $(INC_DIR)code/klcontbl.h $(INC_DIR)lang/klinst.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klerror.pic.o : $(SRC_DIR)error/klerror.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)error/klerror.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klutils.pic.o : $(SRC_DIR)misc/klutils.c $(INC_DIR)misc/klutils.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klcfdarr.pic.o : $(SRC_DIR)parse/klcfdarr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klcfdarr.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klidarr.pic.o : $(SRC_DIR)parse/klidarr.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(INC_DIR)parse/klidarr.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)kllex.pic.o : $(SRC_DIR)parse/kllex.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(INC_DIR)parse/kllex.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)klparser.pic.o : $(SRC_DIR)parse/klparser.c $(DEPS_K_DIR)include/kio/ki.h $(DEPS_K_DIR)include/kio/kio_common.h $(DEPS_K_DIR)include/kio/ko.h $(DEPS_K_DIR)include/array/kgarray.h $(DEPS_K_DIR)include/kutils/utils.h $(DEPS_K_DIR)include/array/karray.h $(INC_DIR)parse/klparser.h $(INC_DIR)ast/klast.h $(INC_DIR)ast/klstrtbl.h $(INC_DIR)misc/klutils.h $(INC_DIR)error/klerror.h $(INC_DIR)lang/kltypes.h $(INC_DIR)parse/kltokens.h $(INC_DIR)parse/kllex.h $(INC_DIR)parse/klcfdarr.h $(INC_DIR)parse/klidarr.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC

$(OBJ_DIR)kltokens.pic.o : $(SRC_DIR)parse/kltokens.c $(INC_DIR)parse/kltokens.h | create_dir
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC





.PHONY: clean create_dir all
clean :
	$(RM) -r $(OBJ_DIR)* $(LIB_DIR)*
	$(MAKE) -C $(DEPS_K_DIR) clean

create_dir :
	mkdir -p $(OBJ_DIR) $(LIB_DIR)
