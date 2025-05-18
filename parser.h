#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE 1024

// Instruction memory (externally accessible)
extern short int instruction_memory[MEM_SIZE];
extern int current_address;
extern int instruction_count;

// Instruction opcodes
typedef enum {
    OP_ADD = 0,    // 0000
    OP_SUB,        // 0001
    OP_MUL,        // 0010
    OP_LDI,        // 0011
    OP_BEQZ,       // 0100
    OP_AND,        // 0101
    OP_OR,         // 0110
    OP_JR,         // 0111
    OP_SLC,        // 1000
    OP_SRC,        // 1001
    OP_LB,         // 1010
    OP_SB          // 1011
} Opcode;

// External declaration of opcode names
extern const char* opcode_names[];

// Function prototypes

/**
 * @brief Parses a register string (e.g., "R5") to register index
 * @param reg_str Register string
 * @return Register index (0-63)
 */
int parse_register(const char *reg_str);

/**
 * @brief Parses an immediate value string (e.g., "5" or "-2")
 * @param imm_str Immediate value string
 * @return 6-bit immediate value (sign-extended for negative values)
 */
int parse_immediate(const char *imm_str);

/**
 * @brief Parses a single instruction string and stores it in memory
 * @param instr_str Instruction string (e.g., "ADD R1 R2")
 */
void parse_instruction(const char *instr_str);

/**
 * @brief Prints the current contents of instruction memory
 * Displays address, binary, opcode, operands, and instruction format
 */
void print_memory();

/**
 * @brief Loads instructions from a text file into memory
 * @param filename Path to the input file containing instructions
 */
void load_program_from_file(const char *filename);

#endif // PARSER_H