#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "memory.h"

instruction_memory[];

void parse(char filename[]) {
    FILE *file = fopen("program.txt", "r");
    if (file == NULL) {
        printf("Could not open file.\n");
    }
    char line[256];
    char mnemonic[10], op1[10], op2[10];
    int j = 0;
    while (fgets(line, sizeof(line), file)){
        sscanf(line, "%s %s %s", mnemonic, op1, op2); //opcode R12 Imm/R2        
        char type = 'R';
        short int instruction; 
        if(strcmp(mnemonic,"ADD")){
            instruction = 0x0000;
        }if(strcmp(mnemonic,"SUB")){
            instruction = 0x1000;
        }if(strcmp(mnemonic,"MUL")){
            instruction = 0x2000;
        }if(strcmp(mnemonic,"LDI")){
            instruction = 0x3000;
            type = 'I';
        }if(strcmp(mnemonic,"BEQZ")){
             instruction = 0x4000;
             type = 'I';
        }if(strcmp(mnemonic,"AND")){
             instruction = 0x5000;
        }if(strcmp(mnemonic,"OR")){
             instruction = 0x6000;
        }if(strcmp(mnemonic,"JR")){
             instruction = 0x7000;
        }if(strcmp(mnemonic,"SLC")){
             instruction = 0x8000;
             type = 'I';
        }if(strcmp(mnemonic,"SRC")){
             instruction = 0x9000;
             type = 'I';
        }if(strcmp(mnemonic,"LB")){
             instruction = 0xA000;
             type = 'I';
        }if(strcmp(mnemonic,"SB")){
             instruction = 0xB000;
             type = 'I';
        }     
        short int  num1;
        memmove(op1, op1 + 1, strlen(op1)); //12
        sscanf(op1, "%d", &num1); 
        //i have the number the register now i need to store that value into the instruction
        //so lets say the number is 61 then i need the isntruction to be "0010" opcode then 6 bits to store 
        // num is an int so its 32 thats good i can shift it upwards by 6? so it would become 000000(6 bits of value)000000
        instruction = (num1 << 6) | instruction; //0000()
        char num2; 
        if(type == 'I'){
            sscanf(op2, "%d", &num2);
            instruction = instruction | num2; 
        }else{
            memmove(op2, op2 + 1, strlen(op2));//-12
            sscanf(op2, "%d", &num2);
            instruction = instruction | num2; 
        }
        instruction_memory[j] = instruction;
        j++;
    }