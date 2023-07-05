#include "lexgen/include/lexgen/control.h"



bool kev_lexgen_parse(FILE* input, KevPatternList* list, KevStringFaMap* map);
bool kev_lexgen_generate(KevPatternList* list, KevStringFaMap* map, KevFA* dfa, uint64_t** p_acc_mapping);
bool kev_lexgen_output_dfa(FILE* output, KevFA* dfa, uint64_t* acc_mapping, int* options);
bool kev_lexgen_output_callback(FILE* output, KevPatternList* list, int* options);
bool kev_lexgen_output_info(FILE* output, KevPatternList* list, int* options);
void kev_lexgen_control(KevOptions* options);
