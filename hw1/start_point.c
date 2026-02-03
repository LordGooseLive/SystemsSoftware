/*
Assignment:
vm.c - Implement a P-machine virtual machine

Authors: <Arav Tulsi, Latrell Kong>

Language: C (only)

To Compile:
  gcc -O2 -Wall -std=c11 -o vm vm.c

To Execute (on Eustis):
  ./vm input.txt

where:
  input.txt is the name of the file containing PM/0 instructions;
  each line has three integers (OP L M)

Notes:
  - Implements the PM/0 virtual machine described in the homework
    instructions.
  - No dynamic memory allocation, no pointer arithmetic, and 
    no function-like macros.
  - Does not implement any VM instruction using a separate function.
  - Runs on Eustis.

Class: COP 3402 - Systems Software - Spring 2026

Instructor : Dr. Jie Lin

Due Date: Monday, February 9th, 2026
*/

#include <stdio.h>
#include <stdlib.h>

//Global Variables
int PC = 0; //Program Counter
int BP = 480; //Base Pointer
int SP = 481; //Stack Pointer; grows downwards
int IR [3] = {0, 0, 0};   //Instruction Register
int pas [500] = {0}; // Programme Address Space

//prototypes
int base (int l);
void print (void);


int main(int argc, char *argv[])
{
    printf("argc = %d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    printf("\n");

    FILE *file = NULL;

    if (argc > 1)
    {
        // Opening file for input
        file = fopen(argv[1], "r");
        if(file == NULL)
        {
            printf("input file could not be opened");
            return 1;
        }
    }
    else
    {
        printf("No extra arguments provided.\n");
        printf("Try: ./00_args 123\n");
        return 1;
    }

    // putting all instructions from file into PAS
    int index = 0;
    int op = 0;
    int l = 0;
    int m = 0;

    while(fscanf(file, "%d %d %d", &op, &l, &m) == 3)
    {
        pas[index] = op;
        pas[index + 1] = l;
        pas[index + 2] = m;
        index += 3;
    }

    // printing initial values (nothing in stack)
    printf("intitial values : \n");
    printf("PC=%d BP=%d SP=%d Stack: ", PC, BP, SP);

    int stopCycle = 0;

    while(!stopCycle){
        //Fetch Cycle
            //Copy current instruction from pas to IR
        IR[0] = pas[PC]; //OP
        IR[1] = pas[PC + 1]; //L
        IR[2] = pas[PC + 2]; //M
        PC += 3; //Increment PC

        //Execute Cycle
        if(IR[0] == 2) //OPR
        {
            switch (IR[2]) //M
            {
                case 0: //RTN
                {
                    SP = BP - 1;
                    BP = pas[SP + 2];
                    PC = pas[SP + 3];
                    break;
                }
                case 1: //NEG

                case 2; //ADD

                case 3; //SUB

                case 4; //MULT

                case 5; //DIV

                case 6; //EQUAL

                case 7; //INEQUAL

                case 8; //LESS-THAN

                case 9; //LESS-THAN/EQUAL

                case 10; //GREATER-THAN

                case 11; //GREATER-THAN/EQUAL

            }
        }
        else //everything other than OPR
        {
            switch (IR[0]) //OP
            {
                case 1: //LIT
                {
                    break;
                }

                case 2: //OPR
                {
                    break;
                }

                case 3: //
                {
                    break;
                }

                case 4:
                {
                    break;
                }
            }
        }
    }

    return 0;
}

int base (int l)
{
    int arb = BP;
    while (l > 0)
    {
        arb = pas[arb];
        l--;
    }
    return arb;
}

void print(void)
{

}