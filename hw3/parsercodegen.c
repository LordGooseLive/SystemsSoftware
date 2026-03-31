/*
Assignment:
HW3 - Parser and Code Generator for PL/0

Author(s): Arav Tulsi, Latrell Kong

Language: C (only)

To Compile:
    gcc -O2 -Wall -std=c11 -o parsercodegen parsercodegen.c

To Execute (on Eustis):
    ./parsercodegen <input_file.txt>

where:
    <input_file.txt> is the path to the PL/0 source program

Notes:
    - Single integrated program: scanner + parser + code gen
    - parsercodegen.c accepts ONE command-line argument
    - Scanner runs internally (no intermediate token file)
    - Implements recursive-descent parser for PL/0 grammar
    - Generates PM/0 assembly code (see Appendix A for ISA)
    - All development and testing performed on Eustis

Class: COP 3402 - Systems Software - Spring 2026

Instructor: Dr. Jie Lin

Due Date: See Webcourses for the posted due date and time.
*/

// includes, macros and enums
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
} tokenType;

typedef enum
{
    noError = 0,
    miscError, // general error 
    inputNull, //input file read error
    outputNull, //output file read error
    skipsymPresent, //lexical analysis error

    // Parsing errors:
    periodExpected, // program must end with period
    identifierExpected, // const, var, and read keywords must be followed by identifier
    duplicateSymbolName, //symbol name has already been declared
    constantAssignmentSymbolExpected, //constants must be assigned with =
    numberExpected, // constants must be assigned an integer value
    semicolonExpected, // constant and variable declarations must be followed by a semicolon
    undeclaredIdentifier, // undeclared identifier
    cannotAlterNonVariable, // only variable values may be altered
    nonConstantIdentiferExpected, // assignment statements must use :=
    endExpected, // begin must be followed by end
    thenExpected, // if must be followed by then
    doExpected, // while must be followed by do
    odExpected, // do must be followed by od
    fiExpected, // if-then statement must end with fi
    comparatorExpected, // condition must contain comparison operator
    rightParenthesisExpected, // right parenthesis must follow left parenthesis
    arithmeticSymbolsExpected // arithmetic equations must contain operands, parentheses, numbers, or symbols

    // Code generation errors:
} errorCode;

// Global Variables
FILE *fIn = NULL;      // Points to PL/0 source file
FILE *fOut = NULL; // Output file with machine code and/ or error message
char lexemes[550][550]; // Array of lexemes
char names [550][550];  // Array of names
int tokens[550];        // Array of tokens
int num_lex = 0;        // Number of lexemes/ tokens scanned
int num_names = 0;      // Number of names scanned
int pCurr = 0;          // Index of current token being parsed

// Function prototypes
int streq (char stringA [], char stringB []); // String equal? 1 : 0
int nameExists (char name [], char names[][550], int num_names); // Name present? 1 : 0

int main(int argc, char *argv[])
{
    // Variable declaration and initialisation
    int character = 0;      // Used to parse file 
    int parseFlag = 0;      // Signals if there is an error during parsing

    // --- Validate inputs and output ---
    // Print all command-line arguments
    printf("argc = %d\n", argc);

    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    printf("\n");

    if (argc == 2)  // Ensure only one input is given and is accessible
    {
        // Opening file for input
        fIn = fopen(argv[1], "r");
        fOut = fopen("elf.txt", "w");
        if (fIn == NULL)
        {
            printf("Input file could not be opened\n");
            return errorHandling(inputNull);
        }

        else if (fOut == NULL)
        {
            printf("Output file could not be opened\n");
            return errorHandling(outputNull);
        }
    }

    else    // Print error message if invalid number of arguments given
    {
        if (argc > 2)
        {
            printf("Too many arguments provided.\n");
            printf("Try: ./00_args 123\n");
            return 1;   // Error: too many arguments
        }

        else
        {
            printf("No extra arguments provided.\n");
            printf("Try: ./00_args 123\n");
            return 1;   // Error: missing argument
        }
    }   // End of input validation

    //--- Lexical Analyser---

    /*   - First checks if char is a comment delimiter or invisible character. 
            - skips comments and invisible characters.
        - Next, collects lexemes and attempts to match with token type 
            - If lexeme is alphanumeric but not a reserve word, it is a name. 
            - Stores name to name table if it is new.
            - If lexeme is a number, ensures it is valid then stores.
            - If lexeme is a special symbol, stores.  
    */
    while((character = fgetc(fIn)) != EOF )
    {
        // Skipping if encountering invisible characters
        if(isspace(character))
        {
            continue;
        }

        // Skipping if encountering multi-line comments
        else if(character == '/')
        {
            int temp  = fgetc(fIn);
            int breakLoop = 1; // Flag

            if(temp == '*')
            {
                // Parsing through whole comment
                while(breakLoop == 1)
                {
                    character = fgetc(fIn);

                    // Error catching if comment goes until EOF
                    if(character == EOF)
                    {
                        printf("Error with multi-line comment");
                        breakLoop = 0;
                        continue;
                    }
                        
                    // Checking as might be end of multi-line comment
                    else if(character == '*')
                    {
                        temp = fgetc(fIn);
                        if(temp == '/')
                        {
                            breakLoop = 0;
                            continue;
                        }

                        else
                        {
                            ungetc(temp, fIn);
                        }
                    }

                    // Continuing to next character in comment
                    continue;
                }
                continue;   
            }

            // Going back if not multi-line comment and just a slash
            else
            {
                ungetc(temp, fIn);
                lexemes[num_lex][0] = '/';
                lexemes[num_lex][1] = '\0';
                tokens[num_lex++] = slashsym;
            }
        }

        // If character is a letter(identifier)
        else if(isalpha(character))
        {
            // Initializing first letter of identifier and counter
            int counter = 0;
            lexemes[num_lex][counter++] = character;

            // Getting next character and checking if it is letter/number
            int temp = fgetc(fIn);
            while(temp != EOF && isalnum(temp))
            {
                // Inputting character into string and getting next character
                if(counter < 99)
                {
                  lexemes[num_lex][counter++] = temp;
                }
                temp = fgetc(fIn);
            }
            
            // Adding null char (terminator) to end
            lexemes[num_lex][counter] = '\0';

            // If was not letter/number, ungetting character
            if(temp != EOF)
            {
                ungetc(temp, fIn);
            }

            // Checking if lexeme is a reserved word
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

            else // Not reserved word, so is an identifier
            {
                // If identifier meets length constraint, treat as identifier. Else, skipsym
                if (counter < 12) 
                {   
                    tokens[num_lex] = identsym;

                    // If identifier is not already in name table, add it
                    if(!nameExists(lexemes[num_lex], names, num_names))
                    {
                        strcpy(names[num_names], lexemes[num_lex]);
                        num_names++;
                    }
                }
                
                else
                {
                    tokens[num_lex] = skipsym; // Invalid identifier, so skip
                    return(errorHandling(skipsymPresent));
                }
            }

            // Incrementing length of lexemes and tokens list
            num_lex++;
        }
        
        // If character is a number
        else if(isdigit(character))
        {
            // Initializing counter and first character of number
            int counter = 0;
            lexemes[num_lex][counter++] = character;

            // Getting next character and checking if it is a number
            int temp = fgetc(fIn);

            while(temp != EOF && isdigit(temp))
            {
              if(counter < 99)
              {
                lexemes[num_lex][counter++] = temp;
              }
              temp = fgetc(fIn);
            }

            // has letter flag
            int hasLetter = 0;

            // Error if letter is next to digit
            if(isalpha(temp))
            {
              // error as there is letter next to number
              hasLetter = 1;

              while(temp != EOF && isalnum(temp))
              {
                if(counter < 99)
                {
                  lexemes[num_lex][counter++] = temp;
                }
                temp = fgetc(fIn);
              }
            }

            // Ungetting temp if it was another symbol after number
            if(temp != EOF)
            {
                ungetc(temp, fIn);
            }
            
            // Adding null terminator to number and putting token in if valid
            lexemes[num_lex][counter] = '\0';

            // skipsym if number has letters
            if(hasLetter == 1)
            {
                tokens[num_lex++] = skipsym;
                printf("Error: Scanning error detected by lexer (skipsym present)\n");
                return(errorHandling(skipsymPresent));
            }
            else if (counter < 6) // Valid number, so add token
            {
                tokens[num_lex++] = numbersym;
            }
            else
            {
                //printf("Number is too long\n");
                tokens[num_lex++] = skipsym; // Invalid number, so skip
                printf("Error: Scanning error detected by lexer (skipsym present)\n");
                return(errorHandling(skipsymPresent));
            }
        }

        // Checking for special symbols
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

        // If encounters '<', using if and else statement to determine which special symbol
        else if(character == '<')
        {
            int temp = fgetc(fIn);

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
                ungetc(temp, fIn);
                lexemes[num_lex][0] = character;
                lexemes[num_lex][1] = '\0';
                tokens[num_lex++] = lessym; 
            }
        }

        // If encounters '>', using if and else statement to determine which special symbol
        else if(character == '>')
        {
            int temp = fgetc(fIn);

            if(temp == '=')
            {
                lexemes[num_lex][0] = '>';
                lexemes[num_lex][1] = '=';
                lexemes[num_lex][2] = '\0';
                tokens[num_lex++] = geqsym;
            }

            else 
            {
                ungetc(temp, fIn);
                lexemes[num_lex][0] = character;
                lexemes[num_lex][1] = '\0';
                tokens[num_lex++] = gtrsym;
            }
        }
        
        // If encounters ':', using if and else statement to determine if it is becomessym
        else if(character == ':')
        {
            int temp = fgetc(fIn);

            if(temp == '=')
            {
                lexemes[num_lex][0] = ':';
                lexemes[num_lex][1] = '=';
                lexemes[num_lex][2] = '\0';
                tokens[num_lex++] = becomessym;
            }

            else 
            {
                //printf("Invalid symbol (no token representation)\n");
                ungetc(temp, fIn);
            }
        }

        // Else statement to catch any outliers
        else
        {
            printf("Invalid character scanned");
        }
    }

    // --- Parser ---
    // A deterministic, recursive-descent parser for PL/0 grammar.
    // in-progress
    /*char line[550]; // Stores current line of tokens
    char add_token[3]; // Stores token to add to line

    for (int i = 0; i < num_lex; i++) // Loop through tokens until we reach periodsym.
    {
        if (tokens[i] == periodsym) //periodsym indicates end of programme
        {
            break;
        }

        //clear line and add_token
        line[0] = '\0';
        add_token[0] = '\0';

        // Get tokens until semicolonsym is consumed
        while (i < num_lex && tokens[i] != semicolonsym\
        {
            printf("Token: %d, Lexeme: %s\n", tokens[i], lexemes[i]); //for testing
            sprintf(add_token, "%d ", tokens[i]); // Convert token to string and add space
            strcat(line, add_token); // Add token to line
            i++;
        }

        //determine grammar rule for line
        if (strcmp(line,""))
            {}
    }*/

    // Call first Grammar rule to begin parsing
    parseFlag = program();// EDIT when function defined
    if (!parseFlag) // if parseFlag == 0 then no errors
    {
        printf("Parsing successful\n");
    }

    else
    {
        return(errorHandling(parseFlag));
    }

    // --- Code Generator ---
    // Generates PM/0 assembly code
    // to be implemented

    // --- Clean up and return ---
    fclose(fIn);
    fclose(fOut);
    return errorHandling(noError);
}

/*  - called in return eg. "return errorHandling(errorCode, outputFile);"
      where "errorCode" is a value of enum errorCode and outputFile
      is the pointer to the output file (currently fOut)
    - Catches an errorcode thrown by other functions. 
    - Builds a message to print to stdout and elf.txt
    - Prints message and returns errorCode
    - Allows main() to return with errorCode
*/
int errorHandling (int errorCode)
{
    //Variable declaration
    char errorMessage [550] = "\n --- ERROR: ";

    //Error Signaling
    printf("\n********************************");

    //Error detailing
    switch (errorCode)
    {
        case noError:
        {
            strcat(errorMessage, "programme ran successfully");
            break;
        }
        
        case inputNull:
        {
            strcat(errorMessage, "input-file read error");
            break;
        }

        case outputNull:
        {
            strcat(errorMessage, "ouput-file read error");
            break;
        }

        case skipsymPresent:
        {
            strcat(errorMessage, "Scanning error detected by lexer (skipsym present)");
            break;
        }

        case periodExpected:
        {
            strcat(errorMessage, "program must end with period");
            break;
        }

        case identifierExpected:
        {
            strcat(errorMessage, "const, var, and read keywords must be followed by identifier");
            break;
        }
        
        case duplicateSymbolName:
        {
            strcat(errorMessage, "symbol name has already been declared");
            break;
        }

        case constantAssignmentSymbolExpected:
        {
            strcat(errorMessage, "constants must be assigned with =");
            break;
        }

        case numberExpected:
        {
            strcat(errorMessage, "constants must be assigned an integer value");
            break;
        }

        case semicolonExpected:
        {
            strcat(errorMessage, "constant and variable declarations must be followed by a semicolon");
            break;
        }

        case undeclaredIdentifier:
        {
            strcat(errorMessage, "undeclared identifier");
            break;
        }

        case cannotAlterNonVariable:
        {
            strcat(errorMessage, "only variable values may be altered");
            break;
        }

        case nonConstantIdentiferExpected:
        {
            strcat(errorMessage, "assignment statements must use :=");
            break;
        }

        case endExpected:
        {
            strcat(errorMessage, "begin must be followed by end");
            break;
        }

        case thenExpected:
        {
            strcat(errorMessage, "if must be followed by then");
            break;
        }

        case doExpected:
        {
            strcat(errorMessage, "while must be followed by do");
            break;
        }

        case odExpected:
        {
            strcat(errorMessage, "do must be followed by od");
            break;
        }

        case fiExpected:
        {
            strcat(errorMessage, "if-then statement must end with fi");
            break;
        }

        case comparatorExpected:
        {
            strcat(errorMessage, "condition must contain comparison operator");
            break;
        }

        case rightParenthesisExpected:
        {
            strcat(errorMessage, "right parenthesis must follow left parenthesis");
            break;
        }

        case arithmeticSymbolsExpected:
        {
            strcat(errorMessage, "arithmetic equations must contain operands, parentheses, numbers, or symbols");
            break;
        }

        default:
        {
            strcat(errorMessage, "miscellaneour error");
            break;
        }
    }

    //close error message
    strcat(errorMessage, " ---\n");

    //output to stdio and elf.txt
    printf(errorMessage);
    fprintf(fOut, errorMessage);

    //return errorCode to caller return()
    return errorCode;
}

// --- Lexical Analyser helper function definitions ---

// Checks if two strings are identical. Returns true or false
int streq (char stringA [], char stringB []) // String equal? 1 : 0
{
    if (strcmp(stringA, stringB) == 0)
        return 1; //true

    else
        return 0; //false
}

// Checks if the name is already in name table.  Returns true or false
int nameExists (char name [], char names[][550], int num_names) // Name present? 1 : 0
{
    int retval = 0; // False by default

    for (int i = 0; i < num_names; i++) // Find name through itteration
    {
        if (streq (name, names[i]))
        {   
            retval = 1; // Sets true since name is present
            break; // No need to continue since we found the name
        }
    }
    return retval;
}

// --- Parser helper function definitions ---

//emit
void emit (int op, int l, int m)
{
    fprintf(fOut, "%d %d %d\n", op, l, m);
}

//<program> ::= <block> "."
int program ()
{
    int retval = block(); // Return value of program, default to 0 (no error)
    if (retval == 0) // If block is successfully parsed
    {
        if (tokens[pCurr] != periodsym) // Check for periodsym at end of program
        {
            retval = periodExpected; // Error: program must end with period
        }
    }

    pCurr++; //increment iterator

    emit(9,0,3); // HALT

    return retval;  
}

//<block> ::= <const-declaration> <var-declaration> <statement>
int block ()
{
    int retval = constDeclaration();
    if (retval == 0) // If const-declaration is successfully parsed
    {
        retval = varDeclaration();
        if (retval == 0) // If var-declaration is successfully parsed
        {
            retval = statement();
        }
    }

    else
    {
        retval = miscError;
    }
    
    pCurr++; //increment iterator
    return retval;
}

// <const-declaration> ::= [ "const" <ident> "=" <number> { "," <ident> "=" <number> } ";" ]
int constDeclaration ()
{
    
}

//<var-declaration> ::= [ "var" <ident> { "," <ident> } ";" ]
int varDeclaration ()
{

}

/*
<statement> ::= [ <ident> ":=" <expression>
                | "begin" <statement> { ";" <statement> } "end"
                | "if" <condition> "then" <statement> [ "else" <statement> ] "fi"
                | "while" <condition> "do" <statement> "od"
                | "read" <ident>
                | "write" <expression> ]
*/
int statement ()
{

}

// <condition> ::= <expression> <rel-op> <expression>
int condition ()
{

}

// <expression> ::= ["-"] <term> { ("+" | "-") <term> }
int expression ()
{

}

// <term> ::= <factor> { ("*" | "/" ) <factor> }
int term ()
{

}

// <factor> ::= <ident> | <number> | "(" <expression> ")"
int factor ()
{

}

// <number> ::= <digit> { <digit> }
int number ()
{

}

// <ident> ::= <letter> { <letter> | <digit> }
int identifier ()
{

}

// <rel-op> ::= "=" | "<>" | "<" | "<=" | ">" | ">="
int relOp ()
{

}

// <digit> ::= "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
int digit ()
{

}

// <letter> ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
int letter ()
{
    
}

// --- Code Generator helper function definitions ---