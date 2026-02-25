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
#include <ctype.h>

typedef enum
{
  skipsym = 1,  // Skip / ignore token
  identsym,     // Identifier
  numbersym,    // Number
  beginsym,     // begin
  endsym,       // end
  ifsym,        // if
  fisym,        // fi
  thensym,      // then
  whilesym,     // while
  dosym,        // do
  odsym,        // od
  callsym,      // call
  constsym,     // const
  varsym,       // var
  procsym,      // procedure
  writesym,     // write
  readsym,      // read
  elsesym,      // else
  plussym,      // +
  minussym,     // -
  multsym,      // *
  slashsym,     // /
  eqsym,        // =
  neqsym,       // <>
  lessym,       // <
  leqsym,       // <=
  gtrsym,       // >
  geqsym,       // >=
  lparentsym,   // (
  rparentsym,   // )
  commasym,     // ,
  semicolonsym, // ;
  periodsym,    // .
  becomessym    // :=
} TokenType;

//REMOVE BEFORE SUBMISSION
#define MAX_TOKENS 50
#define MAX_NAMES 50


int main(int argc, char *argv[])
{
  for (int i = 0; i < argc; i++)
  {
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  printf("\n");


  FILE *file = NULL;
  if (argc == 2)
  {
    // Opening file for input
    file = fopen(argv[1], "r");
    if (file == NULL)
    {
      printf("input file could not be opened");
      return 1;
    }
  }

  // error message if not valid number of arguments
  else
  {
    printf("No extra arguments provided.\n");
    printf("Try: ./00_args 123\n");
    return 1;
  }

  // Variable declaration and Initialisation
  char lexemes[50][12];   // array of lexemes
  char names [50][11];  //array of names
  int tokens[50];         // array of tokens
  int num_lex = 0;            // number of lexemes/ tokens scanned
  int character = 0;      // used to parse file 
  int num_names = 0;

  // parsing character by character until end of file
  while((character = fgetc(file)) != EOF )
  {
    
  }

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
