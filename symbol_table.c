#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "first_pass.h"

/* 
 * Add a new symbol (label) to the symbol table.
 * Allocates a node, copies symbol fields, and appends it to the list.
 */
SymbolNode* add_symbol(SymbolNode **head, const char *name, int value, int is_code, int is_data, int is_external, int is_entry) {
    SymbolNode *new_node = (SymbolNode*)malloc(sizeof(SymbolNode));
    SymbolNode *current;
    
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed for symbol %s\n", name);
        exit(1); /* Abort on critical allocation failure. */
    }

    /* Safe copy of symbol name. */
    strncpy(new_node->name, name, MAX_LABEL_LEN - 1);
    new_node->name[MAX_LABEL_LEN - 1] = '\0';
    
    /* Fill all symbol metadata flags. */
    new_node->value = value;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_external = is_external;
    new_node->is_entry = is_entry;
    new_node->next = NULL;

    /* Append new node to end of linked list. */
    if (*head == NULL) {
        *head = new_node;
    } else {
        current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
    
    return new_node;
}

/* 
 * Lookup symbol by name.
 * Returns pointer to matching node, or NULL if not found.
 */
SymbolNode* find_symbol(SymbolNode *head, const char *name) {
    SymbolNode *current = head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* 
 * Update addresses of data symbols at end of first pass.
 * Data image starts after final code image, so data symbols are shifted by ICF.
 */
void update_data_symbols(SymbolNode *head, int icf) {
    SymbolNode *current = head;
    while (current != NULL) {
        /* Update only symbols marked as data. */
        if (current->is_data) {
            current->value += icf;
        }
        current = current->next;
    }
}

/* Free entire symbol table linked list. */
void free_symbols(SymbolNode *head) {
    SymbolNode *current = head;
    while (current != NULL) {
        SymbolNode *next_node = current->next;
        free(current);
        current = next_node;
    }
}
