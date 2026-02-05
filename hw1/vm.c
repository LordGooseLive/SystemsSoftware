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
int pc = 0; //Program Counter
int bp = 480; //Base Pointer
int sp = 481; //Stack Pointer; grows downwards
int ir [3] = {0, 0, 0};   //Instruction Register
int pas [500] = {0}; // Programme Address Space

//prototypes
int base (int l);
void print (void);


int main(int argc, char *argv[])
{

    //Variable Declaration
        // putting all instructions from file into PAS
    int index = 0;
    int op = 0;
    int l = 0;
    int m = 0;
        //others
    int temp = 0;

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

    while(fscanf(file, "%d %d %d", &op, &l, &m) == 3)
    {
        pas[index] = op;
        pas[index + 1] = l;
        pas[index + 2] = m;
        index += 3;
    }

    // printing initial values (nothing in stack)
    printf("intitial values : \n");
    printf("pc=%d bp=%d sp=%d Stack: ", pc, bp, sp);

    int stopCycle = 0;

    while(!stopCycle){
        // Fetch Cycle
            // Copy current instruction from pas to ir
        ir[0] = pas[pc]; //OP
        ir[1] = pas[pc + 1]; //L
        ir[2] = pas[pc + 2]; //M
        pc += 3; //Increment pc

        // Execute Cycle
        if(ir[0] == 2) //OPR
        {
            switch (ir[2]) //M
            {
                case 0: //RTN
                {
                    sp = bp - 1;
                    bp = pas[sp + 2];
                    pc = pas[sp + 3];
                    break;
                }
                case 1: //NEG
                {
                    pas[sp] = 0 - pas[sp];
                    break;
                }
                case 2: //ADD
                {
                    pas[sp-1] = pas[sp-1] + pas[sp];
                    sp = sp - 1;
                    break;
                }
                case 3: //SUB
                {
                    pas[sp-1] = pas[sp-1] - pas[sp];
                    sp = sp - 1;
                    break;
                }
                case 4: //MULT
                {
                    pas[sp-1] = pas[sp-1] * pas[sp];
                    sp = sp - 1;
                    break;
                }
                case 5: //DIV
                {
                    pas[sp-1] = pas[sp-1] / pas[sp];
                    sp = sp - 1;
                    break;
                }
                case 6: //EQUAL
                {
                    pas[sp-1] = (pas[sp-1] == pas[sp]);
                    sp = sp - 1;
                    break;
                }
                case 7: //INEQUAL
                {
                    pas[sp-1] = (pas[sp-1] != pas[sp]);
                    sp = sp - 1;
                    break;
                }
                case 8: //LESS-THAN
                {
                    pas[sp-1] = (pas[sp-1] < pas[sp]);
                    sp = sp - 1;
                    break;
                }
                case 9: //LESS-THAN/EQUAL
                {
                    pas[sp-1] = (pas[sp-1] <= pas[sp]);
                    sp = sp - 1;
                    break;
                }
                case 10: //GREATER-THAN
                {
                    pas[sp-1] = (pas[sp-1] > pas[sp]);
                    sp = sp - 1;
                    break;
                }
                case 11: //GREATER-THAN/EQUAL
                {
                    pas[sp-1] = (pas[sp-1] >= pas[sp]);
                    sp = sp - 1;
                    break;
                }

                default: //ERROR
                {
                    printf("M = %d invalid for OP = 2 [OPR] \n", ir[2]);
                    print();
                    return;
                }
            }
        }

        else if (ir[0] == 9) //OP == SYS
        {
            switch (ir[2]) //M
            {
                case 1: //OutputInt
                {
                    printf("%d\n", pas[sp]);
                    sp++;
                    break;
                }
                
                case 2: //ReadInt
                {
                    sp--;
                    printf("Please enter an integer: ");
                    scanf("%d", &temp);
                    printf("\n Input accepted.\n");
                    pas[sp] = temp;
                    break;
                }
                
                case 3: //Halt
                {
                    stopCycle = 1;
                    break;
                }

                default: //ERROR
                {
                    printf("M = %d invalid for OP = 9 [SYS] \n", ir[2]);
                    print();
                    return;
                }
            }
        }
        else//everything other than OPR and SYS
        {
            switch (ir[0]) //OP
            {
                case 1: //LIT
                {
                    sp--;
                    pas[sp] = ir[2];
                    break;
                }

                case 3: //LOD
                {
                    sp--;
                    pas[sp] = pas[base(n) + ir[2]]
                    break;
                }

                case 4: //STO
                {
                    pas[base(ir[1])+ir[2]] = pas[sp];
                    sp++;
                    break;
                }

                case 5: //CAL
                {
                    pas[sp-1] = base(ir[1]);
                    pas[sp-2] = bp;
                    pas[sp-3] = pc;
                    bp = sp-1;
                    pc = ir[2];
                    break;
                }

                case 6: //INC
                {
                    sp -= ir[2];
                    break;
                }

                case 7: //JMP
                {
                    pc = ir[2]
                    break;
                }

                case 8: //JPC
                {
                    if (pas[sp]==0) 
                    {
                        pc = ir[2];
                    }
                    sp++;
                    break;
                }

                default: //ERROR
                {
                    printf("OP = %d is invalid \n", ir[0]);
                    print();
                    return;
                }
            }
        }
    }

    return 0;
}

int base (int l)
{
    int arb = bp;
    while (l > 0)
    {
        arb = pas[arb];
        l--;
    }
    return arb;
}

void print(void)
{
    //printf("pc=%d bp=%d sp=%d Stack: ", pc, bp, sp);
}
