#ifndef KEVCC_PARGEN_INCLUDE_PARGEN_OPTIONS_H
#define KEVCC_PARGEN_INCLUDE_PARGEN_OPTIONS_H

#define KEV_PARGEN_OPT_HELP           (0)
#define KEV_PARGEN_OPT_NO             (1)

#define KEV_PARGEN_OPT_FALSE          (0)
#define KEV_PARGEN_OPT_TRUE           (1)

#define KEV_PARGEN_INPUT_PATH         (0)
#define KEV_PARGEN_SRC_TMPL_PATH      (1)
#define KEV_PARGEN_INC_TMPL_PATH      (2)
#define KEV_PARGEN_LANG_NAME          (3)
#define KEV_PARGEN_OUT_SRC_PATH       (4)
#define KEV_PARGEN_OUT_INC_PATH       (5)
#define KEV_PARGEN_LRINFO_COLLEC_PATH (6)
#define KEV_PARGEN_LRINFO_SYMBOL_PATH (7)
#define KEV_PARGEN_LRINFO_ACTION_PATH (8)
#define KEV_PARGEN_LRINFO_GOTO_PATH   (9)
#define KEV_PARGEN_STR_NO             (10)

typedef struct tagKevPOptions {
  int opts[KEV_PARGEN_OPT_NO];
  char* strs[KEV_PARGEN_STR_NO];
} KevPOptions;

#endif
