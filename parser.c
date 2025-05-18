#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"



short int instruction_memory[1024];  
int instruction_count = 0;



typedef enum {
    OP_ADD = 0,    // 0000
    OP_SUB,        // 0001
    OP_MUL,        // 0010
    OP_LDI,       // 0011 
    OP_BEQZ,       // 0100
    OP_AND,        // 0101
    OP_OR,         // 0110
    OP_JR,         // 0111
    OP_SLC,        // 1000
    OP_SRC,        // 1001
    OP_LB,        // 1010
    OP_SB         // 1011 
} Opcode;

const char* opcode_names[] = {
    "ADD", "SUB", "MUL", "LDI",
    "BEQZ", "AND", "OR", "JR",
    "SLC", "SRC", "LB", "SB"
};

// Instruction memory (16-bit words)

int current_address = 0;  // Tracks next available memory location

// Helper: Convert register string (e.g., "R5") to 6-bit binary index
int parse_register(const char *reg_str) {
    int reg_num;
    sscanf(reg_str, "R%d", &reg_num);
    return reg_num;

}

// Helper: Convert immediate string (e.g., "5" or "-2") to 6-bit 2's complement
int parse_immediate(const char *imm_str) {
    int imm = atoi(imm_str);
    return imm & 0x3F;  // 6-bit 2's complement
    
}

void parse_instruction(const char *instr_str) {
    char op[10], arg1[10], arg2[10];
    int opcode, r1, r2, imm;

    // Tokenize instruction
    int num_args = sscanf(instr_str, "%s %s %s", op, arg1, arg2);

    // Determine opcode
    if (strcmp(op, "ADD") == 0)      opcode = OP_ADD;
    else if (strcmp(op, "SUB") == 0) opcode = OP_SUB;
    else if (strcmp(op, "MUL") == 0) opcode = OP_MUL;
    else if (strcmp(op, "LDI") == 0) opcode = OP_LDI;
    else if (strcmp(op, "BEQZ") == 0) opcode = OP_BEQZ;
    else if (strcmp(op, "AND") == 0) opcode = OP_AND;
    else if (strcmp(op, "OR") == 0)  opcode = OP_OR;
    else if (strcmp(op, "JR") == 0)  opcode = OP_JR;
    else if (strcmp(op, "SLC") == 0) opcode = OP_SLC;
    else if (strcmp(op, "SRC") == 0) opcode = OP_SRC;
    else if (strcmp(op, "LB") == 0)  opcode = OP_LB;
    else if (strcmp(op, "SB") == 0)  opcode = OP_SB;

    switch (opcode) {
        // R-Format Instructions (OP | R1 | R2)
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_AND:
        case OP_OR:
        case OP_JR:
            r1 = parse_register(arg1);
            r2 = parse_register(arg2);
            instruction_memory[current_address++] = (opcode << 12) | (r1 << 6) | r2;
            break;

        // I-Format Instructions (OP | R1 | IMM)
        case OP_LDI:
        case OP_BEQZ:
        case OP_SLC:
        case OP_SRC:
        case OP_LB:
        case OP_SB:
            r1 = parse_register(arg1);
            imm = parse_immediate(arg2);
            instruction_memory[current_address++] = (opcode << 12) | (r1 << 6) | imm;
            break;

        default:
            fprintf(stderr, "Unhandled opcode: %d\n", opcode);
    }
}




void load_program_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[40];
    while (fgets(line, sizeof(line), file)) {
        instruction_count++;
        line[strcspn(line, "\n")] = '\0';
        parse_instruction(line);
    }
    for(int i = current_address; i < 1024; i++){
        instruction_memory[i] = -1;
    }
   

    fclose(file);
}

