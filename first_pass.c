#include "first_pass.h"
#include <string.h>

/* Legal opcode table for this assembler. */
static OpcodeInfo opcode_table[NUM_OPCODES] = {
    /* name, opcode, funct, num_operands, valid_src_modes, valid_dest_modes */
    {"mov",  0,  0, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    {"cmp",  1,  0, 2, {1, 1, 0, 1}, {1, 1, 0, 1}},
    {"add",  2, 10, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    {"sub",  2, 11, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    {"lea",  4,  0, 2, {0, 1, 0, 0}, {0, 1, 0, 1}},
    {"clr",  5, 10, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"not",  5, 11, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"inc",  5, 12, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"dec",  5, 13, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"jmp",  9, 10, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    {"bne",  9, 11, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    {"jsr",  9, 12, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    {"red", 12,  0, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"prn", 13,  0, 1, {0, 0, 0, 0}, {1, 1, 0, 1}},
    {"rts", 14,  0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {"stop",15,  0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}}
};

/* Return opcode metadata by operation name, or NULL if not found. */
OpcodeInfo* get_opcode_info(const char *name) {
    int i;
    for (i = 0; i < NUM_OPCODES; i++) {
        if (strcmp(opcode_table[i].name, name) == 0) {
            return &opcode_table[i];
        }
    }
    return NULL; 
}

/* First pass: parse, validate, build symbol table, and reserve/encode words. */
int run_first_pass(FILE *am_file, SymbolNode **sym_table, MemoryWord code_image[], MemoryWord data_image[], int *out_ic, int *out_dc, Macro *macro_table) {
    char line[MAX_LINE_LEN];
    int line_number = 0;
    int error_found = 0;
    int IC = INITIAL_IC; 
    int DC = 0;          

    while (fgets(line, sizeof(line), am_file)) {
        char label[MAX_LABEL_LEN];
        char opcode[MAX_LINE_LEN];
        char operands[MAX_LINE_LEN];
        int has_label = 0;
        int is_valid_line;

        line_number++;
        is_valid_line = parse_line(line, label, opcode, operands);

        if (is_valid_line) {
            if (strlen(label) > 0) {
                has_label = 1;
                if (!is_valid_label(label, macro_table)) {
                    fprintf(stderr, "Error in line %d: Invalid label name '%s'.\n", line_number, label);
                    error_found = 1;
                }
            }

            if (strcmp(opcode, ".data") == 0) {
                if (has_label) {
                    if (find_symbol(*sym_table, label)) {
                        fprintf(stderr, "Error in line %d: Duplicate label '%s'.\n", line_number, label);
                        error_found = 1;
                    } else add_symbol(sym_table, label, DC, 0, 1, 0, 0); 
                }
                encode_data_directive(operands, data_image, &DC); 
            }
            else if (strcmp(opcode, ".string") == 0) {
                if (has_label) {
                    if (find_symbol(*sym_table, label)) {
                        fprintf(stderr, "Error in line %d: Duplicate label '%s'.\n", line_number, label);
                        error_found = 1;
                    } else add_symbol(sym_table, label, DC, 0, 1, 0, 0); 
                }
                encode_string_directive(skip_whitespaces(operands), data_image, &DC); 
            }
            else if (strcmp(opcode, ".extern") == 0) {
                add_symbol(sym_table, operands, 0, 0, 0, 1, 0);
            }
            else if (strcmp(opcode, ".entry") == 0) { /* Ignore in first pass */ }
            else {
                OpcodeInfo *op_info = get_opcode_info(opcode);
                int src_mode = -1, dest_mode = -1, L = 1, line_error = 0;
                char src_op[MAX_LINE_LEN], dest_op[MAX_LINE_LEN];
                char *comma_ptr, *start_src, *check_empty;

                src_op[0] = '\0'; dest_op[0] = '\0';

                if (op_info == NULL) {
                    fprintf(stderr, "Error in line %d: Unknown operation '%s'.\n", line_number, opcode);
                    error_found = 1; line_error = 1;
                } else {
                    if (has_label) {
                        if (find_symbol(*sym_table, label)) {
                            fprintf(stderr, "Error in line %d: Duplicate label '%s'.\n", line_number, label);
                            error_found = 1;
                        } else add_symbol(sym_table, label, IC, 1, 0, 0, 0); 
                    }

                    if (op_info->num_operands == 0) {
                        check_empty = skip_whitespaces(operands);
                        if (*check_empty != '\0' && *check_empty != '\n' && *check_empty != '\r') {
                            fprintf(stderr, "Error in line %d: Extraneous text.\n", line_number);
                            error_found = 1; line_error = 1;
                        }
                    }
                    else if (op_info->num_operands == 1) {
                        strcpy(dest_op, skip_whitespaces(operands));
                        trim_trailing_spaces(dest_op);
                        dest_mode = detect_addressing_mode(dest_op);
                    }
                    else if (op_info->num_operands == 2) {
                        comma_ptr = strchr(operands, ','); 
                        if (comma_ptr != NULL) {
                            start_src = skip_whitespaces(operands);
                            strncpy(src_op, start_src, comma_ptr - start_src);
                            src_op[comma_ptr - start_src] = '\0';
                            trim_trailing_spaces(src_op);
                            strcpy(dest_op, skip_whitespaces(comma_ptr + 1));
                            trim_trailing_spaces(dest_op);
                            src_mode = detect_addressing_mode(src_op);
                            dest_mode = detect_addressing_mode(dest_op);
                        } else {
                            fprintf(stderr, "Error in line %d: Missing comma.\n", line_number);
                            error_found = 1; line_error = 1;
                        }
                    }

                    /* Addressing-mode validation block. */
                    if (!line_error) {
                        if (op_info->num_operands == 2 && (src_mode == -1 || dest_mode == -1)) {
                            fprintf(stderr, "Error in line %d: Invalid addressing mode for 2 operands.\n", line_number);
                            error_found = 1; line_error = 1;
                        } else if (op_info->num_operands == 1 && dest_mode == -1) {
                            fprintf(stderr, "Error in line %d: Invalid addressing mode for 1 operand.\n", line_number);
                            error_found = 1; line_error = 1;
                        }
                    }

                    if (!line_error) {
                        if (op_info->num_operands == 2 && (!op_info->valid_src_modes[src_mode] || !op_info->valid_dest_modes[dest_mode])) {
                            fprintf(stderr, "Error in line %d: Illegal addressing mode.\n", line_number);
                            error_found = 1; line_error = 1;
                        } else if (op_info->num_operands == 1 && !op_info->valid_dest_modes[dest_mode]) {
                            fprintf(stderr, "Error in line %d: Illegal addressing mode.\n", line_number);
                            error_found = 1; line_error = 1;
                        }
                    }

                    if (!line_error) {
                        L = calculate_L(op_info->num_operands, src_mode, dest_mode);
                        if (IC + L > MEMORY_SIZE) {
                            fprintf(stderr, "Error in line %d: Overflow.\n", line_number);
                            error_found = 1; line_error = 1;
                        } else {
                            encode_instruction(op_info, src_mode, src_op, dest_mode, dest_op, IC, code_image);
                            IC += L; 
                        }
                    }
                }
            }
        }
    }
    update_data_symbols(*sym_table, IC);
    *out_ic = IC; *out_dc = DC;
    return !error_found;
}


