/*
Assignment:
lex - Lexical Analyzer for PL/0

Author: <Arav Tulsi, Latrell Kong>

Language: C(only)

To Compile:
  gcc  -O2 -std=c11   -o lex lex.c

To Execute (on Eustis):
  ./lex <input file>

where:
  <input file> is the path to the PL/0 source program

Notes:
  - Implement a lexical analyser for the PL/0 language.
  - The program must detect errors such as 
    - numbers longer than five digits
    - identifiers longer than eleven characters
    - invalid characters.
  - The output format must exactly match the specification.
  - Tested on Eustis.

Class: COP 3402 - System Software - Spring 2026

Instructor: Dr. Jie Lin

Due Date: Monday, February 3, 2026
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//REMOVE BEFORE SUBMISSION
#define MAX_TOKENS 50
#define MAX_NAMES 50


int main(int argc, char *argv[])
{
    printf("argc = %d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }

    printf("\n");

    if (argc > 1)
    {
        int x = atoi(argv[1]);   // convert string to int (simple)
        printf("Converted argv[1] to int: %d\n", x);
    }
    else
    {
        printf("No extra arguments provided.\n");
        printf("Try: ./00_args 123\n");
        return 1; //error 1: insufficient arguments
    }

    //Variable declaration and Initialisation
    //assume 50 tokens
    char lexemes [MAX_TOKENS][11] = ""; //array of lexemes
    int tokens [MAX_TOKENS] = 0; //array of tokens
    char names [MAX_NAMES][11] = ""; //array of identsyms
    int num_lex = 0; //number of records in lexeme table
    int num_names = 0; //number of names in name table

    /*
        IMPLEMENT PROGRAMME BODY
    */

    //Print everything
    //Input file  
    printf("Source Program: \n");
    //implement logic

    //lexeme Table
    printf("Lexeme Table:\n");
    printf("lexeme \t token type \n");
    for (int i = 0; i < num_lex; i++)
    {
        printf("%s \t %d \n", lexemes[i], tokens[i]);
    }

    //Name Table
    printf("Name Table:\n");
    printf("Index \t Name\n");
    for (int i = 0; i < num_names; i++)
    {
        printf("%d \t %s \n", i, names[i]);
    }

    return 0;
}
