# Build assembler with strict warning/standard flags required by assignment.
assembler: main.c pre_assembler.c parser.c first_pass.c encoder.c symbol_table.c second_pass.c output.c utils.c first_pass.h
	gcc -Wall -ansi -pedantic main.c pre_assembler.c parser.c first_pass.c encoder.c symbol_table.c second_pass.c output.c utils.c -o assembler

# Remove binary and editor backup files.
clean:
	rm -f assembler *~
