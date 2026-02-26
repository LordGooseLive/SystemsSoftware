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

//includes, macros and enums
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//REMOVE BEFORE SUBMISSION
#define MAX_TOKENS 50
#define MAX_NAMES 50

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

//function prototypes
int streq (char stringA [], char stringB []); //String equal? 1 : 0
int nameExists (char name [], char names[][11], int num_names); //checks if the name is already in name table

//--- programme body ---

int main(int argc, char *argv[])
{
    // Variable declaration and Initialisation
    char lexemes[50][12];   // array of lexemes
    char names [50][11];    // array of names
    int tokens[50];         // array of tokens
    int num_lex = 0;        // number of lexemes/ tokens scanned
    int character = 0;      // used to parse file 
    int num_names = 0;      // number of names scanned
    FILE *file = NULL;      // stores input file

    //--- validate inputs ---

    //print all inputs
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    printf("\n");

    //ensure only one input is given and it can be accessed
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

    // print error message if invalid number of arguments
    else
    {
        printf("No extra arguments provided.\n");
        printf("Try: ./00_args 123\n");
        return 1;
    }

    //--- parsing, character by character, until end of file---

    /*  First checks if char that we collect is a comment delimiter
        or invisible character. skips comments and invisible characters.
        Next, collects lexemes and attempts to match with token type
        using enum TokenType. 
        If lexeme is alphanumeric but not a reserve word, it is a name. 
        Stores name to name table if it is new.
        If lexeme is a number, ensures it is valid then stores.
        if lexeme is a special symbol, stores.  
    */
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
            int breakLoop = 1; //flag

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
                lexemes[num_lex][0] = '/';
                lexemes[num_lex][1] = '\0';
                tokens[num_lex++] = slashsym;
            }
        }

        // if character is a letter(identifier)
        else if(isalpha(character))
        {
            // initializing first letter of identifier and counter
            int counter = 0;
            lexemes[num_lex][counter++] = character;

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
                lexemes[num_lex][counter++] = temp;
                temp = fgetc(file);
            }
            
            // adding null terminator to end
            lexemes[num_lex][counter] = '\0';

            // if was not letter/number, ungetting character
            if(temp != EOF)
            {
                ungetc(temp, file);
            }

            // checking if identifier is a reserved word
            if(streq(lexemes[num_lex], "begin"))
            {
                tokens[num_lex] = beginsym;
            }

            else if(streq(lexemes[num_lex], "end"))
            {
                tokens[num_lex] = endsym;
            }

            else if(streq(lexemes[num_lex], "if"))
            {
                tokens[num_lex] = ifsym;
            }

            else if(streq(lexemes[num_lex], "fi"))
            {
                tokens[num_lex] = fisym;
            }

            else if(streq(lexemes[num_lex], "then"))
            {
                tokens[num_lex] = thensym;
            }

            else if(streq(lexemes[num_lex], "while"))
            {
                tokens[num_lex] = whilesym;
            }

            else if(streq(lexemes[num_lex], "do"))
            {
                tokens[num_lex] = dosym;
            }

            else if(streq(lexemes[num_lex], "od"))
            {
                tokens[num_lex] = odsym;
            }

            else if(streq(lexemes[num_lex], "call"))
            {
                tokens[num_lex] = callsym;
            }

            else if(streq(lexemes[num_lex], "const"))
            {
                tokens[num_lex] = constsym;
            }

            else if(streq(lexemes[num_lex], "var"))
            {
                tokens[num_lex] = varsym;

            }

            else if(streq(lexemes[num_lex], "procedure"))
            {
                tokens[num_lex] = procsym;
            }

            else if(streq(lexemes[num_lex], "write"))
            {
                tokens[num_lex] = writesym;
            }

            else if(streq(lexemes[num_lex], "read"))
            {
                tokens[num_lex] = readsym;
            }

            else if(streq(lexemes[num_lex], "else"))
            {
                tokens[num_lex] = elsesym;
            }

            else
            {
                tokens[num_lex] = identsym;
                // if identifier is not already in name table, add it
                if(!nameExists(lexemes[num_lex], names, num_names))
                {
                    strcpy(names[num_names], lexemes[num_lex]);
                    num_names++;
                }
            }

            // incrementing length of lexemes and tokens list
            num_lex++;
        }
        
        // if character is a number
        else if(isdigit(character))
        {
            // initializing counter and first character of number
            int counter = 0;
            lexemes[num_lex][counter++] = character;

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
                lexemes[num_lex][counter++] = temp;
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
            lexemes[num_lex][counter] = '\0';
            tokens[num_lex++] = numbersym;
        }

        // checking for special symbols
        else if(character == '+')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = plussym;
        }
        else if(character == '-')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = minussym;
        }
        else if(character == '*')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = multsym;
        }
        else if(character == '=')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = eqsym;
        }
        else if(character == '(')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = lparentsym;
        }
        else if(character == ')')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = rparentsym;
        }
        else if(character == ',')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = commasym;
        }
        else if(character == ';')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = semicolonsym;
        }
        else if(character == '.')
        {
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = periodsym;
        }

        // if encounters '<', using if and else statement to determine which special symbol
        else if(character == '<')
        {
            int temp = fgetc(file);

            if(temp == '>')
            {
                lexemes[num_lex][0] = '<';
                lexemes[num_lex][1] = '>';
                lexemes[num_lex][2] = '\0';
                tokens[num_lex++] = neqsym;
            }

            else if(temp == '=')
            {
                lexemes[num_lex][0] = '<';
                lexemes[num_lex][1] = '=';
                lexemes[num_lex][2] = '\0';
                tokens[num_lex++] = leqsym;
            }

            else 
            {
                ungetc(temp, file);
                lexemes[num_lex][0] = character;
                lexemes[num_lex][1] = '\0';
                tokens[num_lex++] = lessym; 
            }
        }

        // if encounters '>', using if and else statement to determine which special symbol
        else if(character == '>')
        {
            int temp = fgetc(file);

            if(temp == '=')
            {
                lexemes[num_lex][0] = '>';
                lexemes[num_lex][1] = '=';
                lexemes[num_lex][2] = '\0';
                tokens[num_lex++] = geqsym;
            }

            else 
            {
                ungetc(temp, file);
                lexemes[num_lex][0] = character;
                lexemes[num_lex][1] = '\0';
                tokens[num_lex++] = gtrsym;
            }
        }
        
        // if encounters ':', using if and else statement to determine if it is becomessym
        else if(character == ':')
        {
            int temp = fgetc(file);

            if(temp == '=')
            {
                lexemes[num_lex][0] = ':';
                lexemes[num_lex][1] = '=';
                lexemes[num_lex][2] = '\0';
                tokens[num_lex++] = becomessym;
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

    //--- Print all data ---

    //Lexeme Table
    printf("Lexeme Table:\n");
    printf("\nlexeme \t token type \n");

    for (int i = 0; i < num_lex; i++)
    {
        printf("%s \t %d \n", lexemes[i], tokens[i]);
    }

    //Name Table
    printf("\nName Table:\n");
    printf("\nIndex \t Name\n");
    for (int i = 0; i < num_names; i++)
    {
        printf("%d \t %s \n", i, names[i]);
    }

    //Token list
    printf("\n Token List:\n\n");
    for (int i = 0; i < num_lex; i++)
    {
        printf("%s", tokens[i]);
    }

    return 0; //everything went well
}

// ---helper function definitions ---

//function to check if two strings are identical. returns true or false
int streq (char stringA [], char stringB []) //String equal? 1 : 0
{
    if (strcmp(stringA, stringB) == 0)
        return 1; //true
    else
        return 0; //false
}

//checks if the name is already in name table. returns true or false
int nameExists (char name [], char names[][11], int num_names) //name present? 1 : 0
{
    int retval = 0; //false by default

    for (int i = 0; i < num_names; i++)
    {
        if (streq (name, names[i]))
        {   
            retval = 1; //sets true since name is present
            break; //no need to continue since we found the name
        }
    }
    return retval;
}