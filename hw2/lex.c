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
  int tokens[50];         // array of tokens
  int len = 0;            // number of lexemes/ tokens scanned
  int character = 0;      // used to parse file 

  // parsing character by character until end of file
  while((character = fgetc(file)) != EOF )
  {
    // skipping if encountering invisible characters
    if(isspace(character))
    {
      continue;
    }

    // skipping if encountering multi-line comments
    else if(character == '/')
    {
      int temp  = fgetc(file);
      int breakLoop = 1;
      if(temp == '*')
      {
        // parsing through whole comment
        while(breakLoop == 1)
        {
          character = fgetc(file);

          // error catching if comment goes until EOF
          if(character == EOF)
          {
            printf("error with multi-line comment");
            breakLoop = 0;
            continue;
          }

          // checking as might be end of multi-line comment
          else if(character == '*')
          {
            temp = fgetc(file);
            if(temp == '/')
            {
              breakLoop = 0;
              continue;
            }
            else
            {
              ungetc(temp, file);
            }
          }

          // continuing to next character in comment
          continue;
        }
        continue;
      }

      // going back if not multi-line comment and just a slash
      else
      {
        ungetc(temp, file);
        lexemes[len][0] = '/';
        lexemes[len][1] = '\0';
        tokens[len++] = slashsym;
      }
    }

    // if character is a letter(identifier)
    else if(isalpha(character))
    {
      // initializing first letter of identifier and counter
      int counter = 0;
      lexemes[len][counter++] = character;

      // getting next character and checking if it is letter/number
      int temp = fgetc(file);
      while(temp != EOF && isalnum(temp))
      {
        // checking length
        if(counter >= 11)
        {
          printf("Identifier is too long");
          break;
        }

        // inputting character into string and getting next character
        lexemes[len][counter++] = temp;
        temp = fgetc(file);
      }
      
      // adding null terminator to end
      lexemes[len][counter] = '\0';

      // if was not letter/number, ungetting character
      if(temp != EOF)
      {
        ungetc(temp, file);
      }

      // checking if identifier is a reserved word
      if(strcmp(lexemes[len], "begin") == 0)
      {
        tokens[len] = beginsym;
      }
      else if(strcmp(lexemes[len], "end") == 0)
      {
        tokens[len] = endsym;
      }
      else if(strcmp(lexemes[len], "if") == 0)
      {
        tokens[len] = ifsym;
      }
      else if(strcmp(lexemes[len], "fi") == 0)
      {
        tokens[len] = fisym;
      }
      else if(strcmp(lexemes[len], "then") == 0)
      {
        tokens[len] = thensym;
      }
      else if(strcmp(lexemes[len], "while") == 0)
      {
        tokens[len] = whilesym;
      }
      else if(strcmp(lexemes[len], "do") == 0)
      {
        tokens[len] = dosym;
      }
      else if(strcmp(lexemes[len], "od") == 0)
      {
        tokens[len] = odsym;
      }
      else if(strcmp(lexemes[len], "call") == 0)
      {
        tokens[len] = callsym;
      }
      else if(strcmp(lexemes[len], "const") == 0)
      {
        tokens[len] = constsym;
      }
      else if(strcmp(lexemes[len], "var") == 0)
      {
        tokens[len] = varsym;
      }
      else if(strcmp(lexemes[len], "procedure") == 0)
      {
        tokens[len] = procsym;
      }
      else if(strcmp(lexemes[len], "write") == 0)
      {
        tokens[len] = writesym;
      }
      else if(strcmp(lexemes[len], "read") == 0)
      {
        tokens[len] = readsym;
      }
      else if(strcmp(lexemes[len], "else") == 0)
      {
        tokens[len] = elsesym;
      }
      else
      {
        tokens[len] = identsym;
      }
      // incrementing length of lexemes and tokens list
      len++;
    }
    
    // if character is a number
    else if(isdigit(character))
    {
      // initializing counter and first character of number
      int counter = 0;
      lexemes[len][counter++] = character;

      // getting next character and checking if it is a number
      int temp = fgetc(file);
      while(temp != EOF && isdigit(temp))
      {
        // checking if number is too long
        if(counter >= 5)
        {
          printf("Number is too long");
          break;
        }

        // putting next character into lexeme
        lexemes[len][counter++] = temp;
        temp = fgetc(file);
      }

      // error if letter is next to digit
      if(isalpha(temp))
      {
        printf("invalid number (has letters)");
      }

      // ungetting temp if it was another symbol after number
      if(temp != EOF)
      {
        ungetc(temp, file);
      }
      
      // adding null terminator to number and putting token in
      lexemes[len][counter] = '\0';
      tokens[len++] = numbersym;
    }

    // checking for special symbols
    else if(character == '+')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = plussym;
    }
    else if(character == '-')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = minussym;
    }
    else if(character == '*')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = multsym;
    }
    else if(character == '=')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = eqsym;
    }
    else if(character == '(')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = lparentsym;
    }
    else if(character == ')')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = rparentsym;
    }
    else if(character == ',')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = commasym;
    }
    else if(character == ';')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = semicolonsym;
    }
    else if(character == '.')
    {
      lexemes[len][0] = character;
      lexemes[len][1] = '\0';
      tokens[len++] = periodsym;
    }

    // if encounters '<', using if and else statement to determine which special symbol
    else if(character == '<')
    {
      int temp = fgetc(file);
      if(temp == '>')
      {
        lexemes[len][0] = '<';
        lexemes[len][1] = '>';
        lexemes[len][2] = '\0';
        tokens[len++] = neqsym;
      }
      else if(temp == '=')
      {
        lexemes[len][0] = '<';
        lexemes[len][1] = '=';
        lexemes[len][2] = '\0';
        tokens[len++] = leqsym;
      }
      else 
      {
        ungetc(temp, file);
        lexemes[len][0] = character;
        lexemes[len][1] = '\0';
        tokens[len++] = lessym; 
      }
    }

    // if encounters '>', using if and else statement to determine which special symbol
    else if(character == '>')
    {
      int temp = fgetc(file);
      if(temp == '=')
      {
        lexemes[len][0] = '>';
        lexemes[len][1] = '=';
        lexemes[len][2] = '\0';
        tokens[len++] = geqsym;
      }
      else 
      {
        ungetc(temp, file);
        lexemes[len][0] = character;
        lexemes[len][1] = '\0';
        tokens[len++] = gtrsym;
      }
    }
    
    // if encounters ':', using if and else statement to determine if it is becomessym
    else if(character == ':')
    {
      int temp = fgetc(file);
      if(temp == '=')
      {
        lexemes[len][0] = ':';
        lexemes[len][1] = '=';
        lexemes[len][2] = '\0';
        tokens[len++] = becomessym;
      }
      else 
      {
        printf("invalid symbol (no token representation)");
        ungetc(file, temp);
      }
    }

    // else statement to catch any outliers
    else
    {
      printf("Invalid character scanned");
    }
  }

  return 0;
}

// print function
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
