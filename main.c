#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "first_pass.h"
#include "output.h"

/* Definition of maximum filename length */
#define MAX_FILENAME_LEN 100

/**
 * Function to process a single assembly file.
 * Handles Pre-processor, First Pass, Second Pass, and Output generation.
 */
void process_file(char *filename) {
    MemoryWord code_image[MEMORY_SIZE];
    MemoryWord data_image[MEMORY_SIZE];
    SymbolNode *sym_table = NULL;
    Macro *macro_table = NULL; /* Added to handle macro data as required by headers */
    int ic = INITIAL_IC, dc = 0;
    FILE *input_as_file, *output_am_file, *am_file_to_read;
    char as_filename[MAX_FILENAME_LEN];
    char am_filename[MAX_FILENAME_LEN];

    /* 1. Prepare filenames and open initial source file */
    sprintf(as_filename, "%s.as", filename);
    sprintf(am_filename, "%s.am", filename);
    
    input_as_file = fopen(as_filename, "r");
    if (!input_as_file) {
        fprintf(stderr, "Error: Could not open input file %s\n", as_filename);
        return;
    }

    output_am_file = fopen(am_filename, "w");
    if (!output_am_file) {
        fprintf(stderr, "Error: Could not create macro expanded file %s\n", am_filename);
        fclose(input_as_file);
        return;
    }

    /* Calling pre-assembler with macro table pointer as required by first_pass.h */
    if (!run_pre_assembler(input_as_file, output_am_file, &macro_table)) {
        fclose(input_as_file);
        fclose(output_am_file);
        free_macros(macro_table); /* Cleanup macros if failed */
        return; 
    }
    
    fclose(input_as_file);
    fclose(output_am_file);

    /* 2. Open the .am file for Pass 1 and Pass 2 */
    am_file_to_read = fopen(am_filename, "r");
    if (!am_file_to_read) {
        fprintf(stderr, "Error: Could not open %s for reading\n", am_filename);
        free_macros(macro_table);
        return;
    }

    /* 3. First Pass - Updated to include macro_table as required by your header */
    if (!run_first_pass(am_file_to_read, &sym_table, code_image, data_image, &ic, &dc, macro_table)) {
        fclose(am_file_to_read);
        free_symbols(sym_table);
        free_macros(macro_table);
        return;
    }

    /* Rewind file for second pass */
    rewind(am_file_to_read);

    /* 4. Second Pass */
    if (!run_second_pass(am_file_to_read, &sym_table, code_image, &ic, filename)) {
        fclose(am_file_to_read);
        free_symbols(sym_table);
        free_macros(macro_table);
        return;
    }

    fclose(am_file_to_read);

    /* 5. Output Generation (only reaches here if no errors found) */
    write_ob_file(filename, code_image, data_image, ic, dc);
    write_ent_file(filename, sym_table);

    /* 6. Final Cleanup for this file */
    free_symbols(sym_table);
    free_macros(macro_table); /* Freeing the macro table to avoid memory leaks */
}

/* Program entry point: process all input source files from command line. */
int main(int argc, char *argv[]) {
    int i;

    /* Verify that at least one filename was provided */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 file2 ...\n", argv[0]);
        return 1;
    }

    /* Iterate through each command-line argument provided */
    for (i = 1; i < argc; i++) {
        process_file(argv[i]);
    }

    return 0;
}
