#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "first_pass.h"

/* Maximum source line length by project spec (80 + newline + terminator). */
#define MAX_LINE_LEN 82

/* Helper function declarations. */
int is_reserved_word(const char *word);
Macro* add_macro(Macro **head, const char *name);
void add_macro_line(Macro *macro, const char *line);
void free_macros(Macro *head);

/* Pre-assembler stage.
 * Receives the source file (.as) and the expanded output file (.am).
 * Returns 1 on success, 0 if errors were detected.
 */
int run_pre_assembler(FILE *input_file, FILE *output_file, Macro **out_macro_table) {    
    /* Keep declarations at function start for strict ANSI C compatibility. */
    char line[MAX_LINE_LEN + 2]; /* Small safety margin for input reads. */
    char first_word[MAX_LINE_LEN];
    char second_word[MAX_LINE_LEN];
    char extra[MAX_LINE_LEN];
    
    Macro *macro_table = NULL;
    Macro *current_macro = NULL;
    Macro *existing_macro = NULL;
    MacroLine *curr_line = NULL;
    
    int in_macro = 0;
    int error_found = 0;
    int line_number = 0;
    int c; /* Used to drain long lines from input buffer. */

    while (fgets(line, sizeof(line), input_file)) {
        line_number++;
        
        /* 1. Check line length validity, including EOF-safe handling. */
        if (strchr(line, '\n') == NULL && strlen(line) == MAX_LINE_LEN - 1) {
            fprintf(stderr, "Error in line %d: Line exceeds maximum length.\n", line_number);
            error_found = 1;
            /* Flush the remainder of an overlong line from the input buffer. */
            while ((c = fgetc(input_file)) != '\n' && c != EOF);
        }
        /* 2. Read first token and handle blank lines. */
        else if (sscanf(line, "%81s", first_word) != 1) {
            if (!in_macro) {
                fputs(line, output_file);
            } else {
                add_macro_line(current_macro, line);
            }
        }
        /* 3. Handle comment lines. */
        else if (first_word[0] == ';') {
            if (!in_macro) {
                fputs(line, output_file);
            } else {
                add_macro_line(current_macro, line);
            }
        }
        /* 4. Process lines while inside a macro definition. */
        else if (in_macro) {
            if (strcmp(first_word, "mcroend") == 0) {
                /* Ensure there is no trailing text after mcroend. */
                if (sscanf(line, "%*s %81s", extra) == 1) {
                    fprintf(stderr, "Error in line %d: Extraneous text after 'mcroend'.\n", line_number);
                    error_found = 1;
                }
                in_macro = 0;
                current_macro = NULL;
            } else {
                /* Store line as part of the current macro body. */
                add_macro_line(current_macro, line);
            }
        }
        /* 5. Expand an existing macro invocation. */
        else if ((existing_macro = find_macro(macro_table, first_word)) != NULL) {
            /* Ensure there is no extra text after macro call. */
            if (sscanf(line, "%*s %81s", extra) == 1) {
                fprintf(stderr, "Error in line %d: Extraneous text after macro call.\n", line_number);
                error_found = 1;
            }
            /* Expand macro: write all stored lines to output. */
            curr_line = existing_macro->lines_head;
            while (curr_line != NULL) {
                fputs(curr_line->line, output_file);
                curr_line = curr_line->next;
            }
        }
        /* 6. Detect start of macro definition. */
        else if (strcmp(first_word, "mcro") == 0) {
            /* Read macro name and verify no third token exists. */
            if (sscanf(line, "%*s %81s %81s", second_word, extra) == 1) {
                if (is_reserved_word(second_word)) {
                    fprintf(stderr, "Error in line %d: Macro name '%s' is reserved.\n", line_number, second_word);
                    error_found = 1;
                }
                else if (find_macro(macro_table, second_word) != NULL) {
                    fprintf(stderr, "Error in line %d: Macro '%s' already defined.\n", line_number, second_word);
                    error_found = 1;
                }
                else {
                    in_macro = 1;
                    current_macro = add_macro(&macro_table, second_word);
                }
            } else if (sscanf(line, "%*s %81s %81s", second_word, extra) >= 2) {
                fprintf(stderr, "Error in line %d: Extraneous text after macro name.\n", line_number);
                error_found = 1;
            } else {
                fprintf(stderr, "Error in line %d: Missing macro name.\n", line_number);
                error_found = 1;
            }
        }
        /* 7. Regular line: copy as-is to output. */
        else {
            fputs(line, output_file);
        }
    }

    /* Export macro table for later validation in first pass. */
    if (out_macro_table != NULL) {
        *out_macro_table = macro_table;
    }    
    return !error_found;
}

/* Returns 1 if macro name is reserved, otherwise 0. */
int is_reserved_word(const char *word) {
    int i;
    const char *reserved[] = {
        "mov", "cmp", "add", "sub", "lea", "clr", "not", "inc", "dec", 
        "jmp", "bne", "jsr", "red", "prn", "rts", "stop",
        ".data", ".string", ".entry", ".extern",
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
        "mcro", "mcroend"
    };
    int num_reserved = sizeof(reserved) / sizeof(reserved[0]);
    
    for (i = 0; i < num_reserved; i++) {
        if (strcmp(word, reserved[i]) == 0) return 1;
    }
    return 0;
}

/* Find macro by name in linked list. */
Macro* find_macro(Macro *head, const char *name) {
    Macro *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}

/* Create a macro and append it to the macro list. */
Macro* add_macro(Macro **head, const char *name) {
    Macro *new_macro = (Macro*)malloc(sizeof(Macro));
    if (!new_macro) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strncpy(new_macro->name, name, MAX_LINE_LEN - 1);
    new_macro->name[MAX_LINE_LEN - 1] = '\0';
    new_macro->lines_head = NULL;
    new_macro->lines_tail = NULL;
    new_macro->next = NULL;

    if (*head == NULL) {
        *head = new_macro;
    } else {
        Macro *curr = *head;
        while (curr->next != NULL) curr = curr->next;
        curr->next = new_macro;
    }
    return new_macro;
}

/* Append one text line to a macro body. */
void add_macro_line(Macro *macro, const char *line) {
    MacroLine *new_line = (MacroLine*)malloc(sizeof(MacroLine));
    if (!new_line) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strncpy(new_line->line, line, MAX_LINE_LEN - 1);
    new_line->line[MAX_LINE_LEN - 1] = '\0';
    new_line->next = NULL;

    if (macro->lines_head == NULL) {
        macro->lines_head = new_line;
        macro->lines_tail = new_line;
    } else {
        macro->lines_tail->next = new_line;
        macro->lines_tail = new_line;
    }
}

/* Free all dynamically allocated macro structures. */
void free_macros(Macro *head) {
    Macro *curr_macro = head;
    while (curr_macro != NULL) {
        Macro *next_macro = curr_macro->next;
        MacroLine *curr_line = curr_macro->lines_head;
        while (curr_line != NULL) {
            MacroLine *next_line = curr_line->next;
            free(curr_line);
            curr_line = next_line;
        }
        free(curr_macro);
        curr_macro = next_macro;
    }
}
