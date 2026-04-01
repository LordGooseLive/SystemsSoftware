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
#define MAX_SYMBOL_TABLE_SIZE 500

typedef enum tokenType
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

typedef enum errorCode
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
    arithmeticSymbolsExpected, // arithmetic equations must contain operands, parentheses, numbers, or symbols
    tooManyInstructions // too many instructions for code array

    // Code generation errors:
    
} errorCode;

typedef enum opCode
{
    LIT = 1,    // push literal
    OPR,        // operation code
    LOD,        // load to stack
    STO,        // store from stack
    CAL,        // call procedure
    INC,        // allocate locals
    JMP,        // unconditional jump
    JPC,        // conditional jump
    SYS         // system instruction

} opCode;

typedef struct symbol
{
    int kind; // const = 1, var = 2, proc = 3
    char name[12]; // name up to 11 chars long
    int val; // number (ASCII value)
    int level; // L level
    int addr; // M address
    int mark; // 0 = in use, 1 = not in use (deleted)
} symbol;

typedef struct instruction
{
    int op; // opcode
    int l; // L
    int m; // M
} instruction;

// Global Variables
FILE *fIn = NULL;       // Points to PL/0 source file
FILE *fOut = NULL;      // Output file with machine code and/ or error message
char lexemes[500][500]; // Array of lexemes
char names [500][500];  // Array of names
int tokens[500];        // Array of tokens
int num_lex = 0;        // Number of lexemes/ tokens scanned
int num_names = 0;      // Number of names scanned
int pCurr = 0;          // Index of current token being parsed
symbol symbolTable[MAX_SYMBOL_TABLE_SIZE];
int symCurr = 0;        // Index of current symbol in symbol table
instruction code[500];  // Array of instructions for code gen
int cx = 0; // Index of current instruction for code gen

// Function prototypes
int streq (char stringA [], char stringB []); // String equal? 1 : 0
int nameExists (char name [], char names[][500], int num_names); // Name present? 1 : 0
void errorHandling (int errorCode); // Catches error code and prints message
int symTableCheck (char name[]); // Checks if symbol is in symbol table and returns index
int emit (int op, int l, int m); // Emits instruction for code gen


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
            errorHandling(inputNull);
        }

        else if (fOut == NULL)
        {
            printf("Output file could not be opened\n");
            errorHandling(outputNull);
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
                    errorHandling(skipsymPresent);
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

            if (counter < 6) // Valid number, so add token
            {
                tokens[num_lex++] = numbersym;
            }
            else
            {
                //printf("Number is too long\n");
                tokens[num_lex++] = skipsym; // Invalid number, so skip
                printf("Error: Scanning error detected by lexer (skipsym present)\n");
                errorHandling(skipsymPresent);
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
            // saving invalid symbols as a lexeme and adding it as a token for parser
            lexemes[num_lex][0] = character;
            lexemes[num_lex][1] = '\0';
            tokens[num_lex++] = skipsym;

            //  printing error statement
            printf("Invalid character scanned");
        }
    }

    // --- Parser and Code Generator ---
    // A deterministic, recursive-descent parser for PL/0 grammar.

    //Call first Grammar rule to begin parsing and generating code
    program();

    // --- Print Assembly Code ---

    // print assembly code to terminal
    printf("Assembly Code:\n");
    printf("+------+-----+---+-----+\n");
    printf("| Line\t| OP\t| L\t| M\t|\n");
    printf("+------+-----+---+-----+\n");

    for (int i = 0; i < cx; i++) //loop through code array and print all
    {
        printf("| %d\t| %s\t|%d\t| %d\t|\n", i, getOpName(code[i].op), code[i].l, code[i].m);
    }

    printf("+------+-----+---+-----+\n\n");

    //print symbol table to terminal
    printf("\nSymbol Table:\n");
    printf("+------+-------------+-------+-------+---------+------+\n");
    printf("| Kind | Name | Value | Level | Address | Mark |\n");
    printf("+------+-------------+-------+-------+---------+------+\n");

    for (int i = 0; i < symCurr; i++)
    {
        printf("| %d\t| %s\t|%d\t| %d\t| %d\t| %d\t|\n", symbolTable[i].kind, symbolTable[i].name, symbolTable[i].val, symbolTable[i].level, symbolTable[i].addr, symbolTable[i].mark);
    }
    printf("+------+-------------+-------+-------+---------+------+\n");

    //print assembly code to elf.txt
    for (int i = 0; i < cx; i++)
    {        
        fprintf(fOut, "%d\t%d\t%d\n", code[i].op, code[i].l, code[i].m);
    }

    // --- Clean up and return ---
    fclose(fIn);
    fclose(fOut);
    errorHandling(noError);
}

/*
    - Catches an errorcode thrown by other functions. 
    - Builds a message to print to stdout and elf.txt
    - Prints message to stdout and elf.txt then exits with error code
*/
void errorHandling (int errorCode)
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

        case tooManyInstructions:
        {
            strcat(errorMessage, "too many instructions for code array");
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

    exit (errorCode);
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
int nameExists (char name [], char names[][500], int num_names) // Name present? 1 : 0
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

// --- Symbol table helper function ---
int symTableLookup (char name[]) //called SymbolTableChecker in documentation
{
    for (int i = symCurr - 1; i > 0; i--)
    {
        if (streq(name, symbolTable[i].name) && symbolTable[i].mark == 0)
        {
            return i; // Return index of symbol in symbol table
        }
    }
    return -1; // Symbol not found
}

int symTableInsert (int kind, char name[], int val, int level, int addr)
{
    if (symCurr < MAX_SYMBOL_TABLE_SIZE && symTableCheck(name) == -1) // Ensure there is space and name not already present
    {
        symbolTable[symCurr].kind = kind;
        strcpy(symbolTable[symCurr].name, name);
        symbolTable[symCurr].val = val;
        symbolTable[symCurr].level = level;
        symbolTable[symCurr].addr = addr;
        symbolTable[symCurr].mark = 0; // Mark as in use
        symCurr++;
        return 0; // Success
    }
    else
    {
        errorHandling(miscError); // add to error enum
    }
}

int symTableMark (char name [])
{
    if (symTableLookup(name) != -1)
    {
        symbolTable[symTableLookup(name)].mark = 1; // Mark  as not in use
        return 0; // Success
    }
    else
    {
        errorHandling(miscError); // add to error enum
    }
}

// --- Parser helper function definitions ---

//<program> ::= <block> "."
void program ()
{
    block(); // Return value of program, default to 0 (no error)
    
    if (tokens[pCurr] != periodsym) // Check for periodsym at end of program
    {
        errorHandling(periodExpected); // Error: program must end with period
    }

    pCurr++; //increment iterator

    emit(9,0,3); // HALT
}

//<block> ::= <const-declaration> <var-declaration> <statement>
void block ()
{
    constDeclaration();
    int numVars = varDeclaration();
    emit (6, 0, numVars + 3);
    statement();
}

// <const-declaration> ::= [ "const" <ident> "=" <number> { "," <ident> "=" <number> } ";" ]
int constDeclaration ()
{
    // initializing for name and value of symbol
    char identName[12];
    int value = 0;

    // if it is a constant
    if(tokens[pCurr] == constsym)
    {   
        // do-while loop
        do
        {   
            // error check for identifier
            pCurr++;
            if(tokens[pCurr] != identsym)
            {
                return identifierExpected;
            }

            // error check for duplicate symbol
            if(symbolTableCheck(lexemes[pCurr]) != -1)
            {
                return duplicateSymbolName;
            }
            // getting name of symbol
            strcpy(identName, lexemes[pCurr++]);

            // error check for certain symbol
            if(tokens[pCurr] != eqsym)
            {
                return constantAssignmentSymbolExpected;
            }
            pCurr++;

            // error check for number
            if(tokens[pCurr] != numbersym)
            {
                return numberExpected;
            }
            // getting number
            value = atoi(lexemes[pCurr]);

            // adding to symbol table
            symbolTableAdd(1, identName, value, 0, 0);
            pCurr++;
        } while (tokens[pCurr] == commasym);
        // loops if encounters a comma
        
        // error check for semicolon
        if(tokens[pCurr] != semicolonsym)
        {
            return semicolonExpected;
        }

        // incrementing counter
        pCurr++;
    }
}

//<var-declaration> ::= [ "var" <ident> { "," <ident> } ";" ]
int varDeclaration ()
{
    // checking if current token is a variable
    int numVars = 0;
    if(tokens[pCurr] == varsym)
    {
        do
        {
            // error check for identifier
            pCurr++;
            if(tokens[pCurr] != identsym)
            {
                return identifierExpected;
            }
            // error check for duplicate symbol
            if(symbolTableCheck(lexemes[pCurr]) != -1)
            {
                return duplicateSymbolName;
            }
            // adding to symbol table
            symbolTableAdd(2, lexemes[pCurr], 0, 0, numVars + 3);
            numVars++;
            pCurr++;
        } while(tokens[pCurr] == commasym);
        // looping again if encountering comma

        // error check for semi colon
        if(tokens[pCurr] != semicolonsym)
        {
            return semicolonExpected;
        }
        pCurr++;
    }
    return numVars;
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
    // checking if is identifier
    if(tokens[pCurr] == identsym)
    {   
        // storing index of identifier
        int symIdx = symbolTableCheck(lexemes[pCurr]);

        // error check for undeclared identifier
        if(symIdx == -1)
        {
            return undeclaredIdentifier;
        }
        // error check for non-variable
        if(symbolTable[symIdx].kind != 2)
        {
            return cannotAlterNonVariable;
        }
        pCurr++;
        
        // error check for nonconstant identifier
        if(tokens[pCurr] != becomessym)
        {
            return nonConstantIdentiferExpected;
        }
        pCurr++;

        // parsing and generating code from expression
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting STO
        emit(STO, 0, symbolTable[symIdx].addr);
        return 0;
    }

    // checking if were at a beginning
    if(tokens[pCurr] == beginsym)
    {   
        // loop at least once, continue while current token is semi-colon
        do
        {
            pCurr++;
            // parsing and generating code from statement
            int retval = statement();
            if(retval != 0)
            {
                return retval;
            }
        } while(tokens[pCurr] == semicolonsym);

        // check if token is an ending, error if not
        if(tokens[pCurr] != endsym)
        {
            return endExpected;
        }

        pCurr++;
        return 0;
    }

    // checking if current token is an if
    if(tokens[pCurr] == ifsym)
    {
        // incrementing and parsing and generating code for condition
        pCurr++;
        condition();

        // emitting JPC instruction 
        int jpcIdx = cx;
        emit(8, 0, 0);

        // error if does not encounter then
        if(tokens[pCurr] != thensym)
        {
            return thenExpected;
        }
        pCurr++;

        // parsing statement and generating code, then filling in the jump address
        int retval = statement();
        if(retval != 0)
        {
            return retval;
        }

        code[jpcIdx].m = cx;
        return 0;
    }

    // checking if encounter while
    if(tokens[pCurr] == whilesym)
    {   
        // incrementing token and saving current index as start of loop
        pCurr++;
        int loopIdx = cx;

        // parsing and generating condition and error check for do
        condition();
        if(tokens[pCurr] != dosym)
        {
            return doExpected;
        }

        // incrementing token 
        pCurr++;

        // saving current index, and emitting JPC
        int jpcIdx = cx;
        emit(JPC, 0, 0);

        // parsing statement and generating code
        int retval = statement();
        if(retval != 0)
        {
            return retval;
        }

        // emitting JMP and updating JPC instruction
        emit(JMP, 0, loopIdx);
        code[jpcIdx].m = cx;
        return 0;
    }

    // encountering read
    if(tokens[pCurr] == readsym)
    {   
        // incrementing token and error check for identifier
        pCurr++;
        if(tokens[pCurr] != identsym)
        {
            return identifierExpected;
        }

        // getting index for identifier and error check if undeclared
        int symIdx = symbolTableCheck(lexemes[pCurr]);
        if(symIdx == -1)
        {
            return undeclaredIdentifier;
        }

        // error check for unalterable variable
        if(symbolTable[symIdx].kind != 2)
        {
            return cannotAlterNonVariable;
        }
        pCurr++;

        // emitting SYS and STO
        emit(9, 0, 1);
        emit(4, 0, symbolTable[symIdx].addr);
        return 0;
    }

    // encountering write
    if(tokens[pCurr] == writesym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting SYS
        emit(9, 0, 2);
        return 0;
    }
}

// <condition> ::= <expression> <rel-op> <expression>
int condition ()
{   
    // parsing and generating code from expression
    int retval = expression();
    if(retval != 0)
    {
        return retval;
    }

    // encountering equal
    if(tokens[pCurr] == eqsym)
    {   
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting EQL
        emit(2, 0, 8);
    }
    // encounter not equal
    else if(tokens[pCurr] == neqsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting NEQ
        emit(2, 0, 9);
    }

    // encountering less than
    else if(tokens[pCurr] == lessym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting LSS
        emit(2, 0, 10);
    }

    // encounter less than or equal
    else if(tokens[pCurr] == leqsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting LEQ
        emit(2, 0, 11);
    }

    //  encounter greater than
    else if(tokens[pCurr] == gtrsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting GTR
        emit(2, 0, 12);
    }

    // encounter greater than or equal
    else if(tokens[pCurr] == geqsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // emitting GEQ
        emit(2, 0, 13);
    }
    // error check for comparator expected
    else
    {
        return comparatorExpected;
    }

}

// <expression> ::= ["-"] <term> { ("+" | "-") <term> }
int expression ()
{
    // parsing and generating code for term
    int retval = term();
    if(retval != 0)
    {
        return retval;
    }

    // loop while current token is plus or sym
    while(tokens[pCurr] == plussym || tokens[pCurr] == minussym)
    {
        // if plus
        if(tokens[pCurr] == plussym)
        {
            // incrementing token and parsing and generating code for term
            pCurr++;
            int retval = term();
            if(retval != 0)
            {
                return retval;
            }

            // emitting ADD
            emit(2, 0, 2);
        }
        else
        {   
            // incrementing token and parsing and generate code for term
            pCurr++;
            int retval = term();
            if(retval != 0)
            {
                return retval;
            }

            // emitting SUB
            emit(2, 0, 3);
        }
    }

}

// <term> ::= <factor> { ("*" | "/" ) <factor> }
int term ()
{   
    // parsing and generating code for factor
    int retval = factor();
    if(retval != 0)
    {
        return retval;
    }

    // loop while current token is mult or div
    while(tokens[pCurr] == multsym || tokens[pCurr] == slashsym)
    {
        // if mult
        if(tokens[pCurr] == multsym)
        {
            // increment token and parse and generate code for factor
            pCurr++;
            int retval = factor();
            if(retval != 0)
            {
                return retval;
            }

            // emitting MUL
            emit(2, 0, 4);
        }
        else
        {
            // increment token and parse and generate code for factor
            pCurr++;
            int retval = factor();
            if(retval != 0)
            {
                return retval;
            }

            // emitting DIV
            emit(2, 0, 5);
        }
    }
}

// <factor> ::= <ident> | <number> | "(" <expression> ")"
int factor ()
{
    // encounter identifier
    if(tokens[pCurr] == identsym)
    {   
        // storing index of identifier
        int symIdx = symbolTableCheck(lexemes[pCurr]);
        if(symIdx == -1)
        {
            return undeclaredIdentifier;
        }
        // if identifier is a const
        if(symbolTable[symIdx].kind == 1)
        {
            // emitting LIT
            emit(1, 0, symbolTable[symIdx].val);
        }

        // if identifier is var
        else
        {
            // emitting LOD 
            emit(3, 0, symbolTable[symIdx].addr);
        }
        pCurr++;
    }

    // encountering number
    else if(tokens[pCurr] == numbersym)
    {
        // emitting LIT
        emit(1, 0, atoi(lexemes[pCurr++]));
    }

    // encountering left parenthesis
    else if(tokens[pCurr] == lparentsym)
    {
        // incrementing token and parsing and generating code for expression
        pCurr++;
        int retval = expression();
        if(retval != 0)
        {
            return retval;
        }

        // error check for right parenthesis expected
        if(tokens[pCurr] != rparentsym)
        {
            return rightParenthesisExpected;
        }
        pCurr++;
    }

    // error check for arithmetic symbol expected
    else
    {
        return arithmeticSymbolsExpected;
    }
}

// --- Code Generator helper function definitions ---

int emit (int op, int l, int m)
{
    printf(fOut, "%d %d %d\n", op, l, m); // Debugging 


    if (cx < 500)
    {
        code[cx].op = op;
        code[cx].l = l;
        code[cx].m = m;
        cx++;
    }

    else
    {
        errorHandling(tooManyInstructions); // Error: too many instructions
    }
}

// --- Other helper function definitions ---
char* getOpName (int op)
{
    char opName[4];
    switch (op)
    {
        case LIT:
        {
            strcpy(opName, "LIT");
            break;
        }

        case OPR:
        {    
            strcpy(opName, "OPR");
            break;
        }

        case LOD:
        {
            strcpy(opName, "LOD");
            break;
        }

        case STO:
        {    
            strcpy(opName, "STO");
            break;
        }

        case CAL:
        {    
            strcpy(opName, "CAL");
            break;
        }

        case INC:
        {    
            strcpy(opName, "INC");
            break;
        }

        case JMP:
        {    
            strcpy(opName, "JMP");
            break;
        }
        case JPC:
        {    
            strcpy(opName, "JPC");
            break;
        }
        case SYS:
        {    
            strcpy(opName, "SYS");
            break;
        }

        default:
        {    
            strcpy(opName, "ERR"); // Error: invalid OP code
            break;
        }
    }

    return opName;
}