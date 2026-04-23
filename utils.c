#include <stdio.h>
#include <stdlib.h>

/* Skip leading spaces and tabs and return first non-space character. */
char *skip_whitespaces(char *str){
while(*str != '\0' && (*str == ' ' || * str == '\t')){
  str ++;
  }
   return str;
}

/* Parse register name (r0-r7) and return its numeric index, or -1 if invalid. */
int get_register_number(char *str) {
    /* Check if the string is not empty and has a minimum length */
    int r;
    if (str != NULL && *str != '\0') {
         /* The first character must be the lowercase letter 'r' */
        if (*str != 'r') {
            return -1;
        }
        /* Convert the second character (the digit) to an integer */
        r = *(str + 1) - '0';
        /* * Validation:
         * 1. The digit must be between 0 and 7.
         * 2. The string must end immediately after the digit (length must be 2).
         */
        if (r < 0 || r > 7 || *(str + 2) != '\0') {
            return -1;
        }
         return r; /* Valid register found */
    }
    return -1; /* Not a valid register */
}

/* Detect addressing mode by operand syntax.
 * 0 = immediate (#num), 1 = direct label, 2 = relative (%label), 3 = register.
 */
int detect_addressing_mode(char *str) {
    /* Initialize mode to -1 (Invalid) in case no condition is met */
    int mode = -1;
  if (str != NULL && *str != '\0') {
        /* Case 0: Immediate addressing */
        if (*str == '#') {
            mode = 0;
        }
        /* Case 2: Indirect register addressing */
        else if (*str == '%') {
                mode = 2;
        }
        /* Case 3: Direct register addressing */
        else if (get_register_number(str) != -1) {
            /* get_register_number already checks for 'r' and the digit */
            mode = 3;
        }
        /* Case 1: Direct addressing (Label) */
        else {
            /* If it's not #, not *, and not a register, it's a label */
            mode = 1;
        }
    }
     return mode;
}
