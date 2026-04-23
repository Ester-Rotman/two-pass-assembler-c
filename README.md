# Two-Pass Assembler in C

A course-style two-pass assembler implementation in ANSI C.

This project reads one or more assembly source files (`.as`), runs a pre-assembler macro expansion stage, performs first and second pass processing, and emits standard assembler output files.

## Highlights

- Two-pass architecture:
  - Pass 1: symbol collection, directive handling, and initial encoding
  - Pass 2: symbol resolution, `.entry` handling, external reference emission
- Pre-assembler macro expansion with `mcro` / `mcroend`
- Output generation for object and symbol-related files
- Strict compilation flags used by the assignment:
  - `-Wall -ansi -pedantic`

## Repository Structure

- `main.c`: program entry, file orchestration
- `pre_assembler.c`: macro parsing and expansion into `.am`
- `first_pass.c`: first-pass parsing/encoding and symbol table population
- `second_pass.c`: symbol resolution and `.ext` generation
- `output.c`: `.ob` and `.ent` writers
- `parser.c`: line parsing and addressing helpers
- `symbol_table.c`: symbol table operations
- `encoder.c`, `utils.c`: encoding and utility helpers
- `first_pass.h`, `output.h`: public headers
- `makefile`: build and clean targets

## Build

Using `make` (recommended):

```bash
make
```

This produces an executable named `assembler`.

> On Windows, you can build with MinGW/WSL/Git-Bash toolchains that provide `make` and `gcc`.

## Usage

Run with one or more file base names (without `.as`):

```bash
./assembler file1 file2
```

Example using the included sample file `ps.as`:

```bash
./assembler ps
```

## Output Files

For each input `name.as`, the assembler may produce:

- `name.am`: macro-expanded source (pre-assembler output)
- `name.ob`: object file
- `name.ent`: entries file (created only if `.entry` symbols exist)
- `name.ext`: extern usages file (created only if external symbols are referenced)

## Notes

- Input line length is validated according to assignment constraints.
- The assembler uses a memory model starting from address 100 (`INITIAL_IC`).
- The project is intended to be straightforward and educational rather than framework-heavy.

## Clean

```bash
make clean
```

Removes the executable and editor backup files (`*~`).
