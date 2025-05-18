#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"

void print_binary(uint32_t x, int width) {
    for (int bit = width - 1; bit >= 0; bit--) {
        putchar((x & (1u << bit)) ? '1' : '0');
        if (bit % 4 == 0 && bit != 0) putchar(' ');
    }
}

const char* instruction_to_string(short int instruction) {
    static char buffer[20];  // Static buffer to hold the string
    
    uint8_t opcode = (instruction >> 12) & 0xF;
    uint8_t r1 = (instruction >> 6) & 0x3F;
    uint8_t r2_or_imm = instruction & 0x3F;
    
    if (opcode <= OP_JR && opcode != OP_BEQZ && opcode != OP_LDI) {
        snprintf(buffer, sizeof(buffer), "%s R%d R%d", 
                opcode_names[opcode], r1, r2_or_imm);
    }
    else {
        int imm = r2_or_imm;
        if ((imm & 0x20)) {
            imm |= 0xFFFFFFC0;
        }
        
  
        if (opcode == OP_LDI) {
            snprintf(buffer, sizeof(buffer), "LDI R%d %d", r1, imm);
        }
        else {
            snprintf(buffer, sizeof(buffer), "%s R%d %d", 
                    opcode_names[opcode], r1, imm);
        }
    }
    
    return buffer;
}

void log_stage(const char* name, short int instr, short int pc, char fields[4], bool ctrls[5], bool valid) {
    printf("[%s]: ", name);
    if (!valid) {
        printf("No valid instruction\n\n");
        return;
    }
    printf("Instruction=0x%04x  %s\n\n", instr, instruction_to_string(instr));
    printf("Inputs: PC=%d OPC=%d R1=%d R2=%d IMM=%d ", pc, fields[0], fields[1], fields[2], fields[3]);
    printf("C=%d V=%d N=%d S=%d Z=%d\n\n", ctrls[0], ctrls[1], ctrls[2], ctrls[3], ctrls[4]);
    printf("Outputs: ");
    if (strcmp(name, "FETCH") == 0) {
        printf("Instruction=0x%04x, Next PC=%d\n\n", instr, pc);
    } else if (strcmp(name, "DECODE") == 0) {
        printf("OPC=%d, R1=%d, R2=%d, IMM=%d, C=%d, V=%d, N=%d, S=%d, Z=%d\n\n",
               fields[0], fields[1], fields[2], fields[3], ctrls[0], ctrls[1], ctrls[2], ctrls[3], ctrls[4]);
    } else if (strcmp(name, "EXECUTE") == 0) {
        printf("Updated flags: C=%d, V=%d, N=%d, S=%d, Z=%d\n\n", ctrls[0], ctrls[1], ctrls[2], ctrls[3], ctrls[4]);
    }
}

int clock_cycles = 0;

short int IFID[2]; // pc and instruction
bool fetch_valid = false;

char IDIE[4]; // opcode, reg1, reg2, immediate
short int IDIEPC; // PC from IFID
short int IDIEINST; // instruction from IFID for logging
bool IDIEctrls[5]; // C, V, N, S, Z
bool decode_valid = false;

bool flushfetch = false;
bool flushdecode = false;
bool execute_valid = false;


char data_memory[2048];
char GPR[64];
char SREG = 0;
short int PC = 0;

void fetch() {
    if (instruction_memory[PC] == -1) {
        return;
    }
    short int instruction = 0;
    bool was_valid = false;

    if (flushfetch) {
        IFID[0] = 0;
        IFID[1] = 0;
        flushfetch = false;
        fetch_valid = false;
    } else {
        instruction = instruction_memory[PC];
        PC++;
        IFID[0] = instruction;
        IFID[1] = PC;
        fetch_valid = true;
        was_valid = true;
    }

    log_stage("FETCH", IFID[0], IFID[1], IDIE, IDIEctrls, was_valid);
}

void decode() {
    char fields[4] = {0, 0, 0, 0};
    bool ctrls[5] = {0, 0, 0, 0, 0};
    bool was_valid = false;

    if (flushdecode) {
        IDIE[0] = 0;
        IDIE[1] = 0;
        IDIE[2] = 0;
        IDIE[3] = 0;
        IDIEctrls[0] = 0;
        IDIEctrls[1] = 0;
        IDIEctrls[2] = 0;
        IDIEctrls[3] = 0;
        IDIEctrls[4] = 0;
        flushdecode = false;
        decode_valid = false;
    } else if (!fetch_valid) {
        decode_valid = false;
    } else {
        short int instruction = IFID[0];
        char opcode = (instruction >> 12) & 15;
        char reg1 = (instruction >> 6) & 63;
        char reg2 = instruction & 63;
        char immediate = instruction & 63;
        IDIEPC = IFID[1];
        IDIEINST = IFID[0];
        IDIE[0] = opcode;
        IDIE[1] = reg1;
        IDIE[2] = reg2;
        IDIE[3] = immediate;
        IDIEctrls[0] = SREG & 16 ? 1 : 0;
        IDIEctrls[1] = SREG & 8 ? 1 : 0;
        IDIEctrls[2] = SREG & 4 ? 1 : 0;
        IDIEctrls[3] = SREG & 2 ? 1 : 0;
        IDIEctrls[4] = SREG & 1 ? 1 : 0;
        decode_valid = true;
        was_valid = true;

        fields[0] = opcode;
        fields[1] = reg1;
        fields[2] = reg2;
        fields[3] = immediate;
        ctrls[0] = IDIEctrls[0];
        ctrls[1] = IDIEctrls[1];
        ctrls[2] = IDIEctrls[2];
        ctrls[3] = IDIEctrls[3];
        ctrls[4] = IDIEctrls[4];
    }

    log_stage("DECODE", IFID[0], IFID[1], fields, ctrls, was_valid);
}

void execute() {
    bool was_valid = false;
    bool new_ctrls[5] = {IDIEctrls[0], IDIEctrls[1], IDIEctrls[2], IDIEctrls[3], IDIEctrls[4]};

    if (!decode_valid) {
        log_stage("EXECUTE", IDIEINST, IDIEPC, IDIE, new_ctrls, false);
        return;
    }

    char temp = 0;
    char opcode = IDIE[0];
    char reg1adr = IDIE[1];
    char reg2adr = IDIE[2];
    char immediate = IDIE[3];
    char reg1 = GPR[reg1adr];
    char reg2 = GPR[reg2adr];
    bool C = IDIEctrls[0];
    bool V = IDIEctrls[1];
    bool N = IDIEctrls[2];
    bool S = IDIEctrls[3];
    bool Z = IDIEctrls[4];
    was_valid = true;

    switch (opcode) {
        case 0: // ADD
            printf("Performing ADD: %d + %d\n\n", reg1, reg2);
            temp = reg1 + reg2;
            printf("Result: %d\n\n", temp);
            if (((reg1 & 128) == 0) && ((reg2 & 128) == 0)) {
                C = (temp & 128) ? 1 : 0;
            }
            if ((reg1 & 128) == (reg2 & 128)) {
                V = ((temp & 128) != (reg1 & 128)) ? 1 : 0;
            } else {
                V = 0;
            }
            N = (temp & 128) ? 1 : 0;
            S = N ^ V;
            Z = (temp == 0) ? 1 : 0;
            GPR[reg1adr] = temp;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, temp);
            break;

        case 1: // SUB
            printf("Performing SUB: %d - %d\n\n", reg1, reg2);
            temp = reg1 - reg2;
            printf("Result: %d\n\n", temp);
            if ((reg1 & 128) != (reg2 & 128)) {
                V = ((temp & 128) == (reg2 & 128)) ? 1 : 0;
            } else {
                V = 0;
            }
            N = (temp & 128) ? 1 : 0;
            S = N ^ V;
            Z = (temp == 0) ? 1 : 0;
            GPR[reg1adr] = temp;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, temp);
            break;

        case 2: // MUL
            printf("Performing MUL: %d * %d\n\n", reg1, reg2);
            reg1 = reg1 * reg2;
            N = (reg1 & 128) ? 1 : 0;
            Z = (reg1 == 0) ? 1 : 0;
            printf("Result: %d \n\n", reg1);
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, reg1);
            break;

        case 3: // LDI
            printf("Loading Immediate: R%d = %d\n\n", reg1adr, immediate);
            GPR[reg1adr] = immediate;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, immediate);
            break;

        case 4: // BEQZ
            printf("Branch if R%d == 0 (%d)?\n\n ", reg1adr, reg1);
            if (reg1 == 0) {
                PC = IDIEPC + immediate;
                flushdecode = true;
                flushfetch = true;
                decode_valid = false;
                execute_valid = false;
                printf("[EXECUTE]: Branch taken, PC updated to %d\n\n", PC);
            }
            break;

        case 5: // AND
            printf("Performing AND: %d & %d\n\n", reg1, reg2);
            reg1 = reg1 & reg2;
            N = (reg1 & 128) ? 1 : 0;
            Z = (reg1 == 0) ? 1 : 0;
            GPR[reg1adr] = reg1;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, reg1);
            break;

        case 6: // OR
            printf("Performing OR: %d | %d\n\n", reg1, reg2);
            reg1 = reg1 | reg2;
            N = (reg1 & 128) ? 1 : 0;
            Z = (reg1 == 0) ? 1 : 0;
            GPR[reg1adr] = reg1;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, reg1);
            break;

        case 7: // JR
            printf("Jump Register: Combining R%d (%d) and R%d (%d) to set PC\n\n", reg1adr, reg1, reg2adr, reg2);
            PC = ((short int)reg1 << 8) | (reg2 & 0xFF);
            flushdecode = true;
            flushfetch = true;
            decode_valid = false;
            execute_valid = false;
            printf("[EXECUTE]: Jump taken, PC updated to %d\n\n", PC);
            break;

        case 8: // SLC
            printf("Rotate Left: R%d << %d\n\n", reg1adr, immediate);
            reg1 = (reg1 << immediate) | (reg1 >> (8 - immediate));
            N = (reg1 & 128) ? 1 : 0;
            Z = (reg1 == 0) ? 1 : 0;
            GPR[reg1adr] = reg1;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, reg1);
            break;

        case 9: // SRC
            printf("Rotate Right: R%d >> %d\n\n", reg1adr, immediate);
            reg1 = (reg1 >> immediate) | (reg1 << (8 - immediate));
            N = (reg1 & 128) ? 1 : 0;
            Z = (reg1 == 0) ? 1 : 0;
            GPR[reg1adr] = reg1;
            printf("[EXECUTE]: Updated GPR[R%d] = %d\n\n", reg1adr, reg1);
            break;

        case 10: // LB
            printf("Loading from memory[%d] into R%d\n\n", immediate, reg1adr);
            reg1 = data_memory[immediate];
            GPR[reg1adr] = reg1;
            printf("[EXECUTE]: Updated GPR[R%d] = %d (loaded from data_memory[%d])\n\n", reg1adr, reg1, immediate);
            break;

        case 11: // SB
            printf("Storing R%d (%d) into memory[%d]\n\n", reg1adr, reg1, immediate);
            data_memory[immediate] = reg1;
            printf("[EXECUTE]: Updated data_memory[%d] = %d\n\n", immediate, reg1);
            break;

        default:
            printf("[EXECUTE]: Invalid opcode: %d\n\n", opcode);
            break;
    }

    new_ctrls[0] = C;
    new_ctrls[1] = V;
    new_ctrls[2] = N;
    new_ctrls[3] = S;
    new_ctrls[4] = Z;
    SREG = (C << 4) | (V << 3) | (N << 2) | (S << 1) | Z;
    log_stage("EXECUTE", IDIEINST, IDIEPC, IDIE, new_ctrls, was_valid);
}

void single_cycle() {
    printf("\n\n=== Clock Cycle %d ===\n\n", ++clock_cycles);
    execute();
    decode();
    fetch();
}

void print_final_state() {
    printf("\n\n=== Final State ===\n\n");
    printf("Registers:\n\n");
    for (int i = 0; i < 64; i++) {
        printf("R%-2d = %3d (0x%02x)\n\n", i, (int8_t)GPR[i], (uint8_t)GPR[i]);
    }
    printf("PC  = %d (0x%04x)\n\n", PC, PC);
    printf("SREG = ");
    print_binary(SREG, 8);
    printf(" (C=%d, V=%d, N=%d, S=%d, Z=%d)\n\n",
           (SREG >> 4) & 1, (SREG >> 3) & 1, (SREG >> 2) & 1, (SREG >> 1) & 1, SREG & 1);

    printf("\n\nInstruction Memory (non-zero locations):\n\n");
    for (int i = 0; i < 1024; i++) {
        if (instruction_memory[i] != -1) {
            printf("instruction_memory[%d] = 0x%04x\n\n", i, instruction_memory[i]);
        }
    }

    printf("\n\nData Memory (non-zero locations):\n\n");
    for (int i = 0; i < 2048; i++) {
        if (data_memory[i] != 0) {
            printf("data_memory[%d] = %d (0x%02x)\n\n", i, (int8_t)data_memory[i], (uint8_t)data_memory[i]);
        }
    }
}



void final_print(){
    print_final_state();
    printf("\n\n=== Simulation Complete ===\n\n");
}

int main(void) {
   
    load_program_from_file("program.txt");

    while(instruction_memory[PC] != -1) {
        single_cycle();
    }
    single_cycle(); 
    single_cycle(); // Final cycle to process the last instruction

    final_print();

    return 0;

}