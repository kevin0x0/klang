#ifndef KEVCC_TOKENIZER_GENERATOR_INCLUDE_REGEX_PARSER_REGEX_PARSER_H
#define KEVCC_TOKENIZER_GENERATOR_INCLUDE_REGEX_PARSER_REGEX_PARSER_H

#include "tokenizer_generator/include/general/global_def.h"
#include "tokenizer_generator/include/finite_automaton/finite_automaton.h"

#define KEV_REGEX_ERR_NONE              (0)
#define KEV_REGEX_ERR_INVALID_INPUT     (1)
#define KEV_REGEX_ERR_SYNTAX            (2)
#define KEV_REGEX_ERR_GENERATE          (3)



KevFA* kev_regex_parse(uint8_t* regex);
uint64_t kev_regex_get_error(void);
char* kev_regex_get_info(void);

#endif