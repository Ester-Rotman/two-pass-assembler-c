#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include <stdio.h>

/* --- Global constants (must appear first) --- */
#define MAX_LINE_LEN 82       /* Maximum line length: 80 chars + newline + terminator */
#define MAX_LABEL_LEN 32      /* Maximum label length: 31 chars + terminator */
#define MEMORY_SIZE 4096      /* Virtual memory size in words */
#define INITIAL_IC 100        /* Instruction counter starts at address 100 */

/* --- Opcode table and bit-encoding constants --- */
#define NUM_OPCODES 16        
#define OPCODE_SHIFT 8        
#define FUNCT_SHIFT 4         
#define SRC_MODE_SHIFT 2      
#define MASK_12_BITS 0xFFF    

/* --- Symbol table data structures (must appear before prototypes) --- */
typedef struct SymbolNode {
    char name[MAX_LABEL_LEN];
    int value;           
    int is_code;         
    int is_data;         
    int is_external;     
    int is_entry;        
    struct SymbolNode *next;
} SymbolNode;

/* --- Opcode metadata structure --- */
typedef struct {
    char *name;               
    int opcode;               
    int funct;                
    int num_operands;         
    int valid_src_modes[4];   
    int valid_dest_modes[4];  
} OpcodeInfo;

/* --- Memory word representations --- */
typedef struct {
    unsigned int dest_addressing: 2; 
    unsigned int src_addressing:  2; 
    unsigned int funct:           4; 
    unsigned int opcode:          4; 
} InstructionWord;

typedef struct {
    unsigned int value: 12;  
    char are;                
} MemoryWord;

/* --- Macro data structures --- */
typedef struct MacroLine {
    char line[MAX_LINE_LEN];
    struct MacroLine *next;
} MacroLine;

typedef struct Macro {
    char name[MAX_LINE_LEN];
    MacroLine *lines_head;
    MacroLine *lines_tail;
    struct Macro *next;
} Macro;


/* === Function prototypes === */

/* --- Pre-assembler --- */
int run_pre_assembler(FILE *input_file, FILE *output_file, Macro **out_macro_table);
Macro* find_macro(Macro *head, const char *name);
void free_macros(Macro *head);

/* --- Parser and symbol utilities --- */
int parse_line(char *line, char *label, char *opcode, char *operands);
char *skip_whitespaces(char *str);
void trim_trailing_spaces(char *str);
int get_register_number(char *str);
int detect_addressing_mode(char *str);
int calculate_L(int num_operands, int src_mode, int dest_mode);
int is_valid_label(const char *label, Macro *macro_table);

SymbolNode* add_symbol(SymbolNode **head, const char *name, int value, int is_code, int is_data, int is_external, int is_entry);
SymbolNode* find_symbol(SymbolNode *head, const char *name);
void update_data_symbols(SymbolNode *head, int icf); 
void free_symbols(SymbolNode *head);

OpcodeInfo* get_opcode_info(const char *name);

/* --- First-pass logic and encoding --- */
int run_first_pass(FILE *am_file, SymbolNode **sym_table, MemoryWord code_image[], MemoryWord data_image[], int *out_ic, int *out_dc, Macro *macro_table);
void encode_data_directive(char *operands, MemoryWord data_image[], int *dc_ptr);
void encode_string_directive(char *operands, MemoryWord data_image[], int *dc_ptr);
void encode_instruction(OpcodeInfo *op_info, int src_mode, char *src_op, int dest_mode, char *dest_op, int current_ic, MemoryWord code_image[]);

#endif /* FIRST_PASS_H */
int run_second_pass(FILE *am_file, SymbolNode **sym_table, MemoryWord code_image[], int *out_ic, char *base_filename);

