#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "memory.h"

int clock_cycles = 0;

//ADD R1 -6 
short int IFID[2];
char IDIE[4];
char opcode;
char reg1;
char reg2;
char immediate;
bool IDIEctrls[5];
bool flushfetch = false;
bool flushdecode = false;

    
// Instruction Memory: 1024 * 16 bits (word-addressable) instruction = 16 bits = 1111111111111111
short int instruction_memory[1024];

// Data Memory: 2048 * 8 bits (byte-addressable)
char data_memory[2048];

// General Purpose Registers: 64 * 8 bits
char GPR[64];

// Status Register: 8 bits
char SREG = 0b00011111;
bool C;
bool V;
bool N;
bool S;
bool Z;

// Program Counter: 16 bits
short int PC = 0;
//IF/ID <-- after fetching before decoding 

void fetch() {
    // Fetch the instruction from instruction memory
    if(flushfetch){
        IFID[0] = NULL;
        IFID[1] = NULL;
        flushfetch = false;
        return;
    }
    short int instruction = instruction_memory[PC];
    PC++;
    //IF/ID = PC, Instruction
    IFID[0] = instruction;
    IFID[1] = PC;
}

void decode(short int instruction) {
    // Decode opcode and operands
    if(flushdecode){
        IDIEctrls[0] = NULL;
        IDIEctrls[1] = NULL;
        IDIEctrls[2] = NULL;
        IDIEctrls[3] = NULL;
        IDIEctrls[4] = NULL;
        IDIE[0] = NULL;
        IDIE[1] = NULL;
        IDIE[2] = NULL;
        IDIE[3] = NULL;
        flushdecode = false;
        return;
    }
    char opcode = (instruction >> 12) & 0xF; //0000000000001111
    char reg1 = (instruction >> 6) & 0b00111111;
    char reg2 = instruction & 0b00111111;       
    char immediate = instruction & 0b00111111;
    IDIE[0] = opcode;
    IDIE[1] = reg1;
    IDIE[2] = reg2;
    IDIE[3] = immediate;
    C = SREG & 0b00010000;
    V = SREG & 0b00001000;
    N = SREG & 0b00000100;
    S = SREG & 0b00000010;
    Z = SREG & 0b00000001;
    IDIEctrls[0] = C;
    IDIEctrls[1] = V;
    IDIEctrls[2] = N;
    IDIEctrls[3] = S;
    IDIEctrls[4] = Z;
     
    
    //ID/IE = opcode, reg1,reg2,immediate
    fetch();
    execute();
}

void execute() {
        char temp = 0;
        char opcode = IDIE[0];
        char reg1adr = IDIE[1];
        char reg2adr = IDIE[2];
        char immediate = IDIE[3];
        char reg1 = GPR[reg1adr];
        char reg2 = GPR[reg2adr];
        C = IDIEctrls[0] ;
        V = IDIEctrls[1];
        N = IDIEctrls[2];
        S = IDIEctrls[3];
        Z = IDIEctrls[4];
        

    // Execute based on opcode
    switch (opcode) {
        case 0: // ADD R1 R2
            temp = reg1 + reg2;
            if((reg1 & 128) == (reg2 & 128) == 0){ //2 positive --> -ve
                C = ((temp & 128) == 1) ? 1 : 0;
            }
            if((reg1 & 128) == (reg2 & 128)){
                if((temp & 128) != reg1)
                V = 1; 
            }else{
                V = 0;
            }
            N = !(temp & 128) ? 0 : 1;
            S = N ^ V;
            Z = !temp ? 0 : 1;
            reg1 = temp;
            GPR[reg1adr] = reg1;
            // TODO: update flags C(done), V (done), N(done), S(done), Z(done)
            break;

        case 1: // SUB R1 R2
            temp = reg1 - reg2;

            if((reg1 & 128) != (reg2 & 128)){
                if((temp & 128) == reg2)
                V = 1; 
            }else{
                V = 0;
            }
            N = !(temp & 128) ? 0 : 1;
            S = N ^ V;
            Z = !temp ? 0 : 1;
            reg1 = temp;
            GPR[reg1adr] = reg1;
            // TODO: update flags V(done), N(done), S(done), Z(done)
            break;

        case 2: // MUL R1 R2
            reg1 = reg1 * reg2;
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            // TODO: update flags N(done), Z(done)
            break;

        case 3: // LDI R1 IMM
            GPR[reg1adr] = immediate;

            break;

        case 4: // BEQZ R1 IMM
            if (reg1 == 0)
                PC = PC + immediate;
                flushdecode = true;
                flushfetch = true;
                return;
            break;

        case 5: // AND R1 R2
            reg1 = reg1 & reg2;
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            // TODO: update flags N(done), Z(done)
            break;

        case 6: // OR R1 R2
            reg1 = reg1 | reg2;
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            // TODO: update flags N(done), Z(done)
            break;

        case 7: // JR R1 concatenated|| R2
            short int value = reg1;
            PC = (value << 8) | reg2;
            flushdecode = true;
            flushfetch = true;
            return;
            break;

        case 8: // SLC R1 IMM
            reg1 = (reg1 << immediate) | (reg1 >> (8 - immediate)); //10100111 --> 00111000  | 00000101 = 00111101
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            // TODO: update flags N(done), Z(done)
            break;

        case 9: // SRC R1 IMM
            reg1 = (reg1 >> immediate) | (reg1 << (8 - immediate));
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            // TODO: update flags N(done), Z(done)
            break;

        case 10: // LB R1 ADDRESS

            reg1 = data_memory[immediate];
            GPR[reg1adr] = reg1;
            break;

        case 11: // SB R1 ADDRESS
            data_memory[immediate] = reg1;
            break;

        default:
            printf("Invalid opcode: %d\n", opcode);
            break;
    }
    SREG = C << 4 | V << 3 | N << 2 | S << 1 | Z;
    return;
}

void single_cycle(){
    fetch();
    decode(&IFID);
    exectue(&IDIE);
    return;
}

void run_program() {
    int instruction_count = 7; // example
    clock_cycles = 3 + (instruction_count - 1) * 1;
    printf("Expected clock cycles: %d\n", clock_cycles);

    for (int i = 0; i < instruction_count; i++) {
        fetch();
    }
}




