#ifndef __KEVCC_LEXGEN_LEXER_H
#define __KEVCC_LEXGEN_LEXER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct tagLex {
  uint8_t* buffer;
  size_t curpos;  /* current position */
} Lex;

bool lex_init(Lex* lex, char* filepath);
void lex_destroy(Lex* lex);

#endif

// $ 
// $r varname $;
// $r varname $: template $;
// $e template file path $;
// $c text file path $;
// $b varname $: template $| template $;
// $s varname $= value1 $: template1 $| value2 $: template2 $| value3 $: template3 $;
// $a varname $= value $;
