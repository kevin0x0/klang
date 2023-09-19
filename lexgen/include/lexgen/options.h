#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
/* options */
#define KEV_LEXGEN_OPT_HELP           (0)
#define KEV_LEXGEN_OPT_NO             (1)

/* values */
#define KEV_LEXGEN_OPT_FALSE          (0)
#define KEV_LEXGEN_OPT_TRUE           (1)

/* string parameters */
#define KEV_LEXGEN_INPUT_PATH         (0)
#define KEV_LEXGEN_SRC_TMPL_PATH      (1)
#define KEV_LEXGEN_INC_TMPL_PATH      (2)
#define KEV_LEXGEN_LANG_NAME          (3)
#define KEV_LEXGEN_OUT_SRC_PATH       (4)
#define KEV_LEXGEN_OUT_INC_PATH       (5)
#define KEV_LEXGEN_STR_NO             (6)

/* record options */
typedef struct tagKevLOptions {
  int opts[KEV_LEXGEN_OPT_NO];
  char* strs[KEV_LEXGEN_STR_NO];
} KevLOptions;

#endif
