#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
/* options */
#define KEV_LEXGEN_OPT_TAB_ONLY       (0)
#define KEV_LEXGEN_OPT_HELP           (1)
#define KEV_LEXGEN_OPT_NO             (2)

/* values */
#define KEV_LEXGEN_OPT_FALSE          (0)
#define KEV_LEXGEN_OPT_TRUE           (1)

/* string parameters */
#define KEV_LEXGEN_INPUT_PATH         (0)
#define KEV_LEXGEN_LANG_NAME          (1)
#define KEV_LEXGEN_OUT_SRC_PATH       (2)
#define KEV_LEXGEN_OUT_INC_PATH       (3)
#define KEV_LEXGEN_STR_NO             (4)

/* record options */
typedef struct tagKevOptions {
  int opts[KEV_LEXGEN_OPT_NO];
  char* strs[KEV_LEXGEN_STR_NO];
} KevOptions;

#endif
