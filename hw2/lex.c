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

int main(int argc, char *argv[])
{

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
    if (file == NULL)
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

  // Variable declaration and Initialisation
  char lexemes[50][12];   // array of lexemes
  int tokens[50];         // array of tokens
  int len = 0;            // number of lexemes/ tokens scanned
  int character = 0;      // used to parse file 

  while((character = fgetc(file)) != EOF )
  {
    
  }

  return 0;
}

void print(int tokens[], char lexemes[][12], int len)
{
  // print input file
  printf("Source Program: \n");
  // implement logic

  // print lexeme-token pairs
  printf("Lexeme Table:\n");
  printf("lexeme %t token type");
  for (int i = 0; i < len; i++)
  {
    printf("%s %t %d \n", lexemes[i], tokens[i]);
  }
}
