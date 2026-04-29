#include "first_pass.h"
#include <string.h>
#include <stdlib.h>

/**
 * Helper function to fill missing label addresses in the code image.
 */
int fill_operand_info(char *operand_text, int mode, int offset, int IC, SymbolNode **sym_table, MemoryWord code_image[], FILE **ext_file_ptr, char *base_filename) {
    SymbolNode *temp = NULL;
    int array_index = offset - INITIAL_IC;
    char *clean_name; /* Single declaration at top for ANSI C style. */

    /* Mode 1: Direct Addressing (Label) */
    if (mode == 1) {
        clean_name = skip_whitespaces(operand_text);
trim_trailing_spaces(clean_name); /* Normalize leading whitespace. */
        temp = find_symbol(*sym_table, clean_name);
        if (temp != NULL) {
            code_image[array_index].value = temp->value; 
            
            if (temp->is_external) {
                code_image[array_index].are = 'E';
                code_image[array_index].value = 0;
                
                if (*ext_file_ptr == NULL) {
                    char *full_ext_name = (char *)malloc(strlen(base_filename) + 5);
                    if (full_ext_name != NULL) {
                        sprintf(full_ext_name, "%s.ext", base_filename);
                        *ext_file_ptr = fopen(full_ext_name, "w");
                        free(full_ext_name);
                    }
                }
                
                if (*ext_file_ptr != NULL) {
                    fprintf(*ext_file_ptr, "%s %04d\n", temp->name, offset);
                }
            } else {
                code_image[array_index].are = 'R';
            }
        } else {
            fprintf(stderr, "Error: Label '%s' not found.\n", clean_name);
            return 0;
        }
    }
    /* Mode 2: Relative Addressing (Label with %) */
    else if (mode == 2) {
        clean_name = skip_whitespaces(operand_text); /* Normalize leading whitespace. */
        temp = find_symbol(*sym_table, clean_name + 1);
        if (temp != NULL) {
            /* המרחק מחושב מהכתובת של מילת המידע עצמה (offset) */
            code_image[array_index].value = (temp->value - offset) & 0xFFF; 
            code_image[array_index].are = 'A';
        } else {
            fprintf(stderr, "Error: Label '%s' (Relative) not found.\n", clean_name + 1);
            return 0;
        }
    }
    return 1; 
}

/* Second pass: resolve symbols, finalize operand words, and emit .ext usage entries. */
int run_second_pass(FILE *am_file, SymbolNode **sym_table, MemoryWord code_image[], int *out_ic, char *base_filename) {
    char line[MAX_LINE_LEN];
    int line_number = 0;
    int error_found = 0;
    int IC = INITIAL_IC;
    FILE *ext_file = NULL;

    while (fgets(line, sizeof(line), am_file)) {
        char label[MAX_LABEL_LEN], opcode[MAX_LINE_LEN], operands[MAX_LINE_LEN];
        int is_valid_line;
        OpcodeInfo *op_info;

        line_number++;
        is_valid_line = parse_line(line, label, opcode, operands);

        if (is_valid_line) {
            if (strcmp(opcode, ".entry") == 0) {
                SymbolNode *symbol = find_symbol(*sym_table, operands);
                if (symbol != NULL) {
                    symbol->is_entry = 1;
                } else {
                    fprintf(stderr, "Error in line %d: Entry label '%s' not found.\n", line_number, operands);
                    error_found = 1;
                }
            }
            else if ((op_info = get_opcode_info(opcode)) != NULL) {
                int num_ops = op_info->num_operands;
                int src_mode = -1, dest_mode = -1;

                if (num_ops == 0) {
                    IC += 1;
                }
                else if (num_ops == 1) {
                    dest_mode = detect_addressing_mode(skip_whitespaces(operands));
                    /* אופרנד יחיד תמיד נכתב למילה מיד אחרי ההוראה */
                    if (!fill_operand_info(operands, dest_mode, IC + 1, IC, sym_table, code_image, &ext_file, base_filename)) {
                        error_found = 1;
                    }
                    IC += calculate_L(num_ops, -1, dest_mode);
                }
                else if (num_ops == 2) {
                    char *src_op, *dest_op, *comma;
                    comma = strchr(operands, ',');
                    if (comma != NULL) {
                        *comma = '\0';
                        src_op = skip_whitespaces(operands);
                        dest_op = skip_whitespaces(comma + 1);

                        src_mode = detect_addressing_mode(src_op);
                        dest_mode = detect_addressing_mode(dest_op);

                        /* מילוי אופרנד מקור - תמיד בכתובת IC + 1 */
                        if (src_mode == 1 || src_mode == 2) {
                            if (!fill_operand_info(src_op, src_mode, IC + 1, IC, sym_table, code_image, &ext_file, base_filename)) {
                                error_found = 1;
                            }
                        }

                         האוגרים) */
                        if (dest_mode == 1 || dest_mode == 2) {
                            if (!fill_operand_info(dest_op, dest_mode, IC + 2, IC, sym_table, code_image, &ext_file, base_filename)) {
                                error_found = 1;
                            }
                        }

                        IC += calculate_L(num_ops, src_mode, dest_mode);
                    }
                }
            }
        }
    }

    if (ext_file != NULL) {
        fclose(ext_file); 
    }

    *out_ic = IC;
    return !error_found;
}

