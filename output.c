#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "first_pass.h"
/* Write object file (.ob) with code and data images in assembler format. */


void write_ob_file(char *filename, MemoryWord code_image[], MemoryWord data_image[], int ic, int dc)
{
  FILE *file_ptr;
  int i;
  char *full_name;
  full_name = (char *)malloc((strlen(filename) + 4) * sizeof(char));
  if (full_name == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for filename\n");
        return;
    }
  sprintf(full_name, "%s.ob", filename);
  
  file_ptr = fopen(full_name, "w");
  if (file_ptr == NULL) {
        printf("Error: Could not create file %s\n", full_name);
        free(full_name);
        return;
    }
    
  /* Print the header line: code length and data length */
  fprintf(file_ptr, "%d %d\n", ic - INITIAL_IC, dc);

  /* Loop for code_image */
  for (i = 0; i < (ic - INITIAL_IC); i++) {
    fprintf(file_ptr, "%04d %03X %c\n", 100 + i, code_image[i].value & 0xFFF, code_image[i].are);
  }
  /* Loop for data_image */
  for (i = 0; i < dc; i++) {
    fprintf(file_ptr, "%04d %03X %c\n", ic + i, data_image[i].value & 0xFFF, 'A');
  }
  fclose(file_ptr);
  free(full_name);
}

/* Write entries file (.ent) only if at least one symbol is marked as entry. */
void write_ent_file(char *filename, SymbolNode *head) {
    SymbolNode *temp = head;
    FILE *file_ptr = NULL;
    char *full_name = NULL;

    while (temp != NULL) {
        if (temp->is_entry) {
            if (file_ptr == NULL) {
                full_name = (char *)malloc(strlen(filename) + 5);
                sprintf(full_name, "%s.ent", filename);
                file_ptr = fopen(full_name, "w");
            }
            if (file_ptr != NULL) {
                fprintf(file_ptr, "%s %04d\n", temp->name, temp->value);
            }
             }
            temp = temp->next;
        }

    if (file_ptr != NULL) {
        fclose(file_ptr);
    }
    if (full_name != NULL) {
        free(full_name);
    }
}


