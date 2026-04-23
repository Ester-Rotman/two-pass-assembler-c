#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "first_pass.h"

/* Encode .data operands into data image with strict comma/memory checks. */
void encode_data_directive(char *operands, MemoryWord data_image[], int *dc_ptr) {
    char *ptr = operands;
    char *endptr;
    int state = 0; /* 0 = expecting number, 1 = expecting comma */
    long val;
    int error_found = 0;

    while (*ptr != '\0' && !error_found) {
        
        while (isspace(*ptr) && *ptr != '\0') {
            ptr++;
        }

        if (*ptr != '\0') {
            if (state == 0) {
                if (*ptr == ',') {
                    fprintf(stderr, "Error: Invalid leading or consecutive comma in .data directive.\n");
                    error_found = 1;
                } else {
                    val = strtol(ptr, &endptr, 10);
                    if (ptr == endptr) {
                        fprintf(stderr, "Error: Invalid number format in .data directive.\n");
                        error_found = 1;
                    } else {
                        /* Ensure we do not exceed virtual data memory bounds. */
                        if (*dc_ptr >= MEMORY_SIZE) {
                            fprintf(stderr, "Error: Data memory overflow during .data encoding.\n");
                            error_found = 1;
                        } else {
                            data_image[*dc_ptr].value = val & MASK_12_BITS; 
                            data_image[*dc_ptr].are = 'A';
                            (*dc_ptr)++;

                            ptr = endptr;
                            state = 1;
                        }
                    }
                }
            } else {
                if (*ptr == ',') {
                    state = 0;
                    ptr++;
                } else {
                    fprintf(stderr, "Error: Missing comma between numbers in .data directive.\n");
                    error_found = 1;
                }
            }
        }
    }

    if (!error_found && state == 0 && ptr != operands) {
        fprintf(stderr, "Error: Trailing comma at the end of .data directive.\n");
    }
}

/* Encode .string contents into data image with memory-bound checks. */
void encode_string_directive(char *operands, MemoryWord data_image[], int *dc_ptr) {
    char *p = strchr(operands, '"');
    int error_found = 0;
    
    if (p != NULL) {
        p++; /* Skip opening quote. */
        
        while (*p != '"' && *p != '\0' && !error_found) {
            if (*dc_ptr >= MEMORY_SIZE) {
                fprintf(stderr, "Error: Data memory overflow during .string encoding.\n");
                error_found = 1;
            } else {
                data_image[*dc_ptr].value = *p & MASK_12_BITS;
                data_image[*dc_ptr].are = 'A';
                (*dc_ptr)++;
                p++;
            }
        }
        
        if (!error_found) {
            if (*p == '\0') {
                fprintf(stderr, "Error: Missing closing quote in .string directive.\n");
            } else if (*dc_ptr >= MEMORY_SIZE) {
                fprintf(stderr, "Error: Data memory overflow during .string terminator encoding.\n");
            } else {
                data_image[*dc_ptr].value = 0;
                data_image[*dc_ptr].are = 'A';
                (*dc_ptr)++;
            }
        }
    } else {
        fprintf(stderr, "Error: Missing opening quote in .string directive.\n");
    }
}

/* Encode instruction word and its extra operand words into code image. */
void encode_instruction(OpcodeInfo *op_info, int src_mode, char *src_op, int dest_mode, char *dest_op, int current_ic, MemoryWord code_image[]) {
    unsigned int word = 0;
    int offset = current_ic - INITIAL_IC; 
    
    int reg1, reg2;
    char *endptr;
    long val;

    word |= (op_info->opcode << 8); 
    word |= (op_info->funct << 4);  
    if (src_mode != -1) word |= (src_mode << 2); 
    if (dest_mode != -1) word |= (dest_mode);    

    code_image[offset].value = word & MASK_12_BITS;
    code_image[offset].are = 'A';
    offset++;

    if (op_info->num_operands == 2) {
        if (src_mode == 3 && dest_mode == 3) {
            reg1 = get_register_number(src_op);
            reg2 = get_register_number(dest_op);
            code_image[offset].value = ((1 << reg1) | (1 << reg2)) & 0xFFF;
            code_image[offset].are = 'A';
            offset++;
        } else {
            if (src_mode == 0) { 
                val = strtol(src_op + 1, &endptr, 10); 
                if (*endptr != '\0' && !isspace(*endptr)) {
                    fprintf(stderr, "Error: Invalid immediate number format in source operand.\n");
                }
                code_image[offset].value = val & MASK_12_BITS; 
                code_image[offset].are = 'A';
            } else if (src_mode == 3) { 
                code_image[offset].value = (1 << get_register_number(src_op)) & MASK_12_BITS;
                code_image[offset].are = 'A';
            } else { 
                code_image[offset].value = 0;
                code_image[offset].are = '?'; 
            }
            offset++;

            if (dest_mode == 0) {
                val = strtol(dest_op + 1, &endptr, 10);
                if (*endptr != '\0' && !isspace(*endptr)) {
                    fprintf(stderr, "Error: Invalid immediate number format in destination operand.\n");
                }
                code_image[offset].value = val & MASK_12_BITS;
                code_image[offset].are = 'A';
            } else if (dest_mode == 3) {
                code_image[offset].value = (1 << get_register_number(dest_op)) & MASK_12_BITS;
                code_image[offset].are = 'A';
            } else {
                code_image[offset].value = 0;
                code_image[offset].are = '?';
            }
            offset++;
        }
    } else if (op_info->num_operands == 1) {
        if (dest_mode == 0) {
            val = strtol(dest_op + 1, &endptr, 10);
            if (*endptr != '\0' && !isspace(*endptr)) {
                fprintf(stderr, "Error: Invalid immediate number format in destination operand.\n");
            }
            code_image[offset].value = val & MASK_12_BITS;
            code_image[offset].are = 'A';
        } else if (dest_mode == 3) {
            code_image[offset].value = (1 << get_register_number(dest_op)) & MASK_12_BITS;
            code_image[offset].are = 'A';
        } else {
            code_image[offset].value = 0;
            code_image[offset].are = '?';
        }
        offset++;
    }
}
