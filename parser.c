#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "first_pass.h"

/* Remove trailing whitespace from a mutable string. */

void trim_trailing_spaces(char *str) {
    int len;
    if (str == NULL) return;
    len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

/* Parse one source line into label/opcode/operands fields. */
int parse_line(char *line, char *label, char *opcode, char *operands) {
    char *ptr = line;
    char first_word[MAX_LINE_LEN];

    /* Initialize all outputs to empty strings. */
    label[0] = '\0';
    opcode[0] = '\0';
    operands[0] = '\0';

    /* Skip leading spaces and tabs. */
    ptr = skip_whitespaces(ptr);

    /* Ignore empty lines and comment lines (';'). */
    if (*ptr == '\0' || *ptr == '\n' || *ptr == ';') {
        return 0; /* Nothing to parse for assembler passes. */
    }

    /* Extract first token from line. */
    sscanf(ptr, "%81s", first_word);

    /* If first token ends with ':', it is a label definition. */
    if (first_word[strlen(first_word) - 1] == ':') {
        /* Copy label name without trailing ':'. */
        strncpy(label, first_word, strlen(first_word) - 1);
        label[strlen(first_word) - 1] = '\0';

        /* Move pointer past label and following spaces. */
        ptr += strlen(first_word);
        ptr = skip_whitespaces(ptr);

        /* Allow label-only lines without crashing. */
        if (*ptr == '\0' || *ptr == '\n') return 1;

        /* Next token is opcode/directive. */
        sscanf(ptr, "%81s", opcode);
    } else {
        /* Otherwise the first token is the opcode/directive. */
        strcpy(opcode, first_word);
    }

    /* Move pointer after opcode to read operands. */
    ptr += strlen(opcode);
    ptr = skip_whitespaces(ptr);

    /* Remaining text belongs to the operands field. */
    if (*ptr != '\0' && *ptr != '\n') {
        strcpy(operands, ptr);
        
        /* Strip trailing newline from operands, if present. */
        if (operands[strlen(operands) - 1] == '\n') {
            operands[strlen(operands) - 1] = '\0';
        }
    }

    return 1; /* Parsing completed successfully. */
}

/*
 * Compute instruction length (L) in machine words.
 * Input: operand count (0/1/2) and source/destination addressing modes.
 */
int calculate_L(int num_operands, int src_mode, int dest_mode) {
    int L = 1; /* The instruction word itself always takes one word. */

    if (num_operands == 1) {
        L += 1; /* One extra word for destination operand. */
    }
    else if (num_operands == 2) {
        /* Two register operands (mode 3) share one additional word. */
        if (src_mode == 3 && dest_mode == 3) {
            L += 1;
        } else {
            L += 2; /* Otherwise each operand uses a separate word. */
        }
    }
    
    return L;
}

int is_valid_label(const char *label, Macro *macro_table) {
    int i;
    int num_reserved;
    int is_valid = 1; /* Validation flag: assume valid until a rule fails. */
    
    const char *reserved[] = {
        ".data", ".string", ".entry", ".extern", "mcro", "mcroend"
    };
    
    /* 1. Length validation. */
    if (label == NULL || strlen(label) == 0 || strlen(label) >= MAX_LABEL_LEN) {
        is_valid = 0;
    }
    
    /* 2. First character must be alphabetic. */
    if (is_valid && !isalpha(label[0])) {
        is_valid = 0;
    }
    
    /* 3. Remaining characters must be alphanumeric. */
    if (is_valid) {
        for (i = 1; label[i] != '\0' && is_valid; i++) {
            if (!isalnum(label[i])) {
                is_valid = 0;
            }
        }
    }
    
    /* 4. Reject register names. */
    if (is_valid && get_register_number((char *)label) != -1) {
        is_valid = 0;
    }
    
    /* 5. Reject opcode names. */
    if (is_valid && get_opcode_info(label) != NULL) {
        is_valid = 0;
    }
    
    /* 6. Reject additional reserved words (directives/macro keywords). */
    if (is_valid) {
        num_reserved = sizeof(reserved) / sizeof(reserved[0]);
        for (i = 0; i < num_reserved && is_valid; i++) {
            if (strcmp(label, reserved[i]) == 0) {
                is_valid = 0;
            }
        }
    }
    
    /* 7. Reject names already used by macros. */
    if (is_valid && macro_table != NULL) {
        if (find_macro(macro_table, label) != NULL) {
            is_valid = 0; /* Name collision with existing macro. */
        }
    }
    
    return is_valid; /* 1 = valid label, 0 = invalid. */
}
