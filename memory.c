#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "memory.h"
#include "parser.h"

void print_binary(uint32_t x, int width) {
    for (int bit = width - 1; bit >= 0; bit--) {
        putchar( (x & (1u << bit)) ? '1' : '0' );
        // for readability, you could add a space every 4 bits:
        if (bit % 4 == 0 && bit != 0) putchar(' ');
    }
}

void log_stage(const char* name, short int instr, short int pc, char fields[4],bool ctrls[5]) {
    printf("[%s]: \n", name);
    printf("instruction being %sed -->%d\n",name, instr);
    printf("pc=%d OPC=%d R1=%d R2=%d IMM=%d \n",pc, fields[0], fields[1], fields[2],fields[3]);
    printf("C=%d V=%d N=%d S=%d Z=%d\n",ctrls[0], ctrls[1], ctrls[2], ctrls[3], ctrls[4]); 
}

int clock_cycles = 0;


short int IFID[2];


char IDIE[4];
short int IDIEPC;
short int IDIEINST;

char opcode;
char reg1;
char reg2;
char immediate;
bool IDIEctrls[5];

bool flushfetch = false;
bool flushdecode = false;
bool decodevalid = false;
bool executevalid = false;
    

char data_memory[2048];

char GPR[64];

char SREG = 0b00000000;

short int PC = 0;

void fetch() {
    if(flushfetch){
        IFID[0] = 0;
        IFID[1] = 0;
        flushfetch = false;
        return;
    }
    short int instruction = instruction_memory[PC];
    PC++;
    IFID[0] = instruction;
    IFID[1] = PC;
    log_stage("IF",IFID[0],IFID[1],IDIE, IDIEctrls);
}

void decode() {
    if(flushdecode){
        IDIEctrls[0] = 0;
        IDIEctrls[1] = 0;
        IDIEctrls[2] = 0;
        IDIEctrls[3] = 0;
        IDIEctrls[4] = 0;
        IDIE[0] = 0;
        IDIE[1] = 0;
        IDIE[2] = 0;
        IDIE[3] = 0;
        flushdecode = false;
        decodevalid = false;
        return;
    }
    if(!decodevalid){
        decodevalid = true;
        return;
    }
    short int instruction = IFID[0];
    char opcode = (instruction >> 12) & 0x000F; 
    printf("opcode: %d\n", opcode);
    char reg1 = (instruction >> 6) & 0b00111111;
    char reg2 = instruction & 0b00111111;       
    char immediate = instruction & 0b00111111;
    IDIEPC = IFID[1];
    IDIEINST = IFID[0];
    IDIE[0] = opcode;
    IDIE[1] = reg1;
    IDIE[2] = reg2;
    IDIE[3] = immediate;
    IDIEctrls[0] = SREG & 0b00010000;
    IDIEctrls[1] = SREG & 0b00001000;
    IDIEctrls[2] = SREG & 0b00000100;
    IDIEctrls[3] = SREG & 0b00000010;
    IDIEctrls[4] = SREG & 0b00000001;
    log_stage("DECODE",IFID[0],IFID[1],IDIE, IDIEctrls);
}

void execute() {
    char temp = 0;
    char opcode = IDIE[0];
    char reg1adr = IDIE[1];
    char reg2adr = IDIE[2];
    char immediate = IDIE[3];
    char reg1 = GPR[reg1adr];
    char reg2 = GPR[reg2adr];
    bool C = IDIEctrls[0] ;
    bool V = IDIEctrls[1];
    bool N = IDIEctrls[2];
    bool S = IDIEctrls[3];
    bool Z = IDIEctrls[4];


    if(immediate & 0b00100000){
        immediate = immediate | 0b11000000;
    }
    if(!decodevalid){
        return;
    }
    if(decodevalid && !executevalid){
            executevalid = true;
            return;
    }
    switch (opcode) {
        case 0: 
            temp = reg1 + reg2;
            if((reg1 & 128) == (reg2 & 128) == 0){ 
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
            
            break;

        case 1: 
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
            
            break;

        case 2: 
            reg1 = reg1 * reg2;
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
           
            break;

        case 3:
            GPR[reg1adr] = immediate;

            break;

        case 4: 
            if (reg1 == 0){
                PC = IDIEPC + immediate;
                flushdecode = true;
                flushfetch = true; 
                decodevalid = false;
                executevalid = false;
                return;
            }
            break;

        case 5: 
            reg1 = reg1 & reg2;
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            
            break;

        case 6: 
            reg1 = reg1 | reg2;
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;

            break;

        case 7: 
            short int value = reg1; 
            PC = (value << 8) | reg2;
            flushdecode = true;
            flushfetch = true;
            decodevalid = false;
            executevalid = false;
            return;

        case 8: 
            reg1 = (reg1 << immediate) | (reg1 >> (8 - immediate)); 
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            
            break;

        case 9: 
            reg1 = (reg1 >> immediate) | (reg1 << (8 - immediate));
            N = !(reg1 & 128) ? 0 : 1;
            Z = !reg1 ? 0 : 1;
            GPR[reg1adr] = reg1;
            
            break;

        case 10: 

            reg1 = data_memory[immediate];
            GPR[reg1adr] = reg1;
            break;

        case 11:
            data_memory[immediate] = reg1;
            break;

        default:
            printf("Invalid opcode: %d\n", opcode);
            break;
    }
    SREG = C << 4 | V << 3 | N << 2 | S << 1 | Z;
    log_stage("IE",IDIEINST,IDIEPC,IDIE, IDIEctrls);
    return;
}

void single_cycle(){
    
    execute();
    decode();
    fetch();    
    return;
}

void print_gpr(){
    printf("GPR:\n");
    for(int i = 0; i < 64; i++){
        printf("R%d: %d\n",i,GPR[i]);
    }
}


int main(void) {
   
    load_program_from_file("program.txt");

    for(int i = 0; i < 7+(instruction_count -1); i++){
        single_cycle();
        printf("Clock Cycle: %d\n", clock_cycles ++);
    }

    print_gpr();
    print_binary(PC,16);
    printf("\n %d \n",PC);

    return 0;
}



