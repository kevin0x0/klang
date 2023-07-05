#ifndef KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H
#define KEVCC_LEXGEN_INCLUDE_LEXGEN_OPTIONS_H

/* options */
#define KEV_LEXGEN_OPT_LANGUAGE       (0)
#define KEV_LEXGEN_OPT_PROCEDURE      (1)
#define KEV_LEXGEN_OPT_WIDTH          (2)
#define KEV_LEXGEN_OPT_CHARSET        (3)
#define KEV_LEXGEN_OPT_PUT_CALLBACK   (4)
#define KEV_LEXGEN_OPT_PUT_INFO       (5)
#define KEV_LEXGEN_OPT_NO             (6)

/* values */
#define KEV_LEXGEN_OPT_LANG_C_CPP     (0)
#define KEV_LEXGEN_OPT_LANG_RUST      (1)
#define KEV_LEXGEN_OPT_PROC_TAB       (0)
#define KEV_LEXGEN_OPT_PROC_SRC       (1)
#define KEV_LEXGEN_OPT_PROC_CPL       (2)
#define KEV_LEXGEN_OPT_CHARSET_ASCII  (0)
#define KEV_LEXGEN_OPT_CHARSET_UTF8   (1)
#define KEV_LEXGEN_OPT_FALSE          (0)
#define KEV_LEXGEN_OPT_TRUE           (1)

/* string parameters */
#define KEV_LEXGEN_INPUT_PATH         (0)
#define KEV_LEXGEN_OUTPUT_PATH        (1)
#define KEV_LEXGEN_STR_NO             (2)

typedef struct tagKevOptions {
  int opts[KEV_LEXGEN_OPT_NO];
  char* strs[KEV_LEXGEN_STR_NO];
} KevOptions;

#endif
