#ifndef OUTPUT_H
#define OUTPUT_H
#include "first_pass.h"

/* Output file generation API (.ob and .ent). */
void write_ob_file(char *filename, MemoryWord code_image[], MemoryWord data_image[], int ic, int dc);
void write_ent_file(char *filename, SymbolNode *head);

#endif

