/*
Assignment:
HW4 - Parser/Code Generator Complete Version for PL/0
      (procedures + lexicographical level management)

Author(s): Arav Tulsi, Latrell Kong

Language: C (only)

To Compile:
  gcc -O2 -Wall -std=c11 -o parsercodegen_comp parsercodegen_comp.c

To Execute (on Eustis):
  ./parsercodegen_comp <input_file.txt>

where:
  <input_file.txt> is the path to the PL/0 source program

Notes:
  - Single integrated program: scanner + parser + code gen
  - parsercodegen_comp.c accepts ONE command-line argument
  - Scanner runs internally (no intermediate token file)
  - Implements recursive-descent parser for the full PL/0 grammar
    including procedure declarations and the call statement
  - Tracks lexicographical levels (nesting depth) for every symbol
  - Generates PM/0 assembly code with proper L fields for LOD/STO/CAL
    and emits CAL/RTN instructions for procedures
    (see Appendix A for the ISA)
  - All development and testing performed on Eustis

Class: COP 3402 - Systems Software - Spring 2026

Instructor: Dr. Jie Lin

Due Date: See Webcourses for the posted due date and time.
*/

// --- includes, macros, enums and structs ---
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define MAX 500 // Max number of tokens, lexemes, names, symbols, and instructions

typedef enum tokenType      // Numeric representation of token types for scanner
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

typedef enum errorCode      // Numeric representation of error codes for error handling
{
    noError = 0,
    miscError,                          // general error 
    inputNull,                          //input file read error
    outputNull,                         //output file read error
    skipsymPresent,                     //lexical analysis error
    periodExpected,                     // program must end with period
    identifierExpected,                 // const, var, and read keywords must be followed by identifier
    duplicateSymbolName,                //symbol name has already been declared
    constantAssignmentSymbolExpected,   //constants must be assigned with =
    numberExpected,                     // constants must be assigned an integer value
    semicolonExpected,                  // constant and variable declarations must be followed by a semicolon
    undeclaredIdentifier,               // undeclared identifier
    cannotAlterNonVariable,             // only variable values may be altered
    nonConstantIdentifierSymbolExpected,// assignment statements must use :=
    endExpected,                        // begin must be followed by end
    thenExpected,                       // if must be followed by then
    doExpected,                         // while must be followed by do
    odExpected,                         // do must be followed by od
    fiExpected,                         // if-then statement must end with fi
    comparatorExpected,                 // condition must contain comparison operator
    rightParenthesisExpected,           // right parenthesis must follow left parenthesis
    arithmeticSymbolsExpected,          // arithmetic equations must contain operands, parentheses, numbers, or symbols
    tooManyInstructions,                // too many instructions for code array
    symTableInsertionFailed,            //failed to insert symbol into symbol table
    symTableMarkFailed,                 //failed to mark symbol as not in use in symbol table
    symTableLookupFailed,               //failed to find symbol in symbol table
    nonConstantIdentifierExpected,      // only non-constant identifiers can be assigned a value
    procedureExpected
} errorCode;

typedef enum opCode         // Numeric representation of opcodes for code generation
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

typedef enum oprCode        // Numeric representation of OPR instructions for code generation
{
    //M values for OPP = OPR(2)
    RET = 0,    // return
    NEG,        // negate
    ADD,        // addition
    SUB,        // subtraction
    MUL,        // multiplication
    DIV,        // division
    EQL,        // equality check
    NEQ,        // inequality check
    LSS,        // less than check
    LEQ,        // less than or equal to check
    GTR,        // greater than check
    GEQ         // greater than or equal to check
} oprCode;

typedef enum sysCode        // Numeric representation of SYS instructions for code generation
{
    WRITE = 1,  // write top stack value to output
    READ,       // read input and push to stack
    HALT        // halt the program

} sysCode;

typedef struct symbol       // Struct for storing symbol information
{
    int kind;       // const = 1, var = 2, proc = 3
    char name[12];  // name up to 11 chars long
    int val;        // number (ASCII value)
    int level;      // L level
    int addr;       // M address
    int mark;       // 0 = in use, 1 = not in use (deleted)
} symbol;

typedef struct instruction  // Struct for storing instruction information
{
    int op; // opcode
    int l;  // L
    int m;  // M
} instruction;

// --- Function prototypes ---
int streq (char stringA [], char stringB []);   // String equal? 1 : 0
int nameExists (char name [], char names[][MAX], 
    int num_names);                             // Name present? 1 : 0
int symTableLookup (char name[]);               // Checks if symbol is in symbol table and returns index
void symTableInsert (int kind, char name[], 
    int val, int level, int addr);              // Inserts symbol into symbol table
void symTableMark (symbol* sym);                // Marks symbol as not in use in symbol table
void program ();                                // Grammar rule for <program>
void block ();                                  // Grammar rule for <block>
void constDeclaration ();                       // Grammar rule for <const-declaration>
void procDeclaration();                         // Grammar rule for <procedure-declaration>
int varDeclaration ();                          // Grammar rule for <var-declaration>
void statement ();                              // Grammar rule for <statement>
void condition ();                              // Grammar rule for <condition>
void expression ();                             // Grammar rule for <expression>
void term ();                                   // Grammar rule for <term>
void factor ();                                 // Grammar rule for <factor>
void emit (int op, int l, int m);               // Emits instruction for code gen
void errorHandling (int errorCode);             // Catches error code and prints message
void getOpName (int op, char opName[4]);        // Gets string representation of opcode for printing assembly code

// --- Global Variables ---
int tokens[MAX];        // Array of tokens
int num_lex = 0;        // Number of lexemes/ tokens scanned
int num_names = 0;      // Number of names scanned
int pCurr = 0;          // Index of current token being parsed
int cx = 0;             // Index of current instruction for code gen
int symCurr = 1;        // Index of current symbol in symbol table. Starts at 1 since 0 is sentinel value for symTableLookup failure
char lexemes[MAX][MAX]; // Array of lexemes
char names [MAX][MAX];  // Array of names
FILE *fIn = NULL;       // Points to PL/0 source file
FILE *fOut = NULL;      // Output file with machine code and/ or error message
instruction code[MAX];  // Array of instructions for code gen
symbol symbolTable[MAX];// Array of symbols for symbol table
int currentLevel = 0;   // Index of current level

int main(int argc, char *argv[])
{
    // Variable declaration and initialisation
    int character = 0;      // Used to parse file 

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

    //Call highest level Grammar rule to begin parsing and generating code
    program();

    // --- Print Assembly Code ---

    // print assembly code to terminal
    printf("Assembly Code:\n");
    printf("+-------+-------+-------+-------+\n");
    printf("| Line\t| OP\t| L\t| M\t|\n");
    printf("+-------+-------+-------+-------+\n");

    for (int i = 0; i < cx; i++)    // Loop through code array and print all
    {
        char opName[4];
        getOpName(code[i].op, opName);
        printf("| %d\t| %s\t|%d\t| %d\t|\n", i, opName, code[i].l, code[i].m);
    }

    printf("+-------+-------+-------+-------+\n\n");

    //print symbol table to terminal
    printf("\nSymbol Table:\n");
    printf("+-------+-------+-------+-------+---------------+-------+\n");
    printf("| Kind\t| Name\t| Value\t| Level\t| Address\t| Mark\t|\n");
    printf("+-------+-------+-------+-------+---------------+-------+\n");

    for (int i = 1; i < symCurr; i++)   // Start at 1 since 0 is sentinel value for symTableLookup failure
    {
        printf("| %d\t| %s\t|%d\t| %d\t| %d\t\t| %d\t|\n", symbolTable[i].kind, symbolTable[i].name, symbolTable[i].val, symbolTable[i].level, symbolTable[i].addr, symbolTable[i].mark);
    }
    printf("+-------+-------+-------+-------+---------------+-------+\n");

    //print assembly code to elf.txt
    for (int i = 0; i < cx; i++)
    {        
        fprintf(fOut, "%d\t%d\t%d\n", code[i].op, code[i].l, code[i].m);
    }

    // --- Clean up and return ---

    fclose(fIn);
    fclose(fOut);
    return 0;   // No errors
} // End of main

// --- Lexical Analyser helper function definitions ---

// Checks if two strings are identical. Returns true or false
int streq (char stringA [], char stringB []) // String equal? 1 : 0
{
    if (strcmp(stringA, stringB) == 0)
        return 1; //true

    else
        return 0; //false
} // End of streq()

// Checks if the name is already in name table.  Returns true or false
int nameExists (char name [], char names[][MAX], int num_names) // Name present? 1 : 0
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
} // End of nameExists()

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
} // End of symTableLookup()

void symTableInsert (int kind, char name[], int val, int level, int addr)
{
    // ensuring name is not present at current level
    int symExists = symTableLookup(name);
    if(symExists != -1 && symbolTable[symExists].level == level)
    {
        errorHandling(duplicateSymbolName);
    }

    if (symCurr < MAX)    // Ensure there is space 
    {
        symbolTable[symCurr].kind = kind;
        strcpy(symbolTable[symCurr].name, name);
        symbolTable[symCurr].val = val;
        symbolTable[symCurr].level = level;
        symbolTable[symCurr].addr = addr;
        symbolTable[symCurr].mark = 0;  // Mark as in use
        symCurr++;
    }
    else
    {
        errorHandling(symTableInsertionFailed);
    }
} // End of symTableInsert()

void symTableMark (symbol* sym)
{
    if (sym != NULL)
    {
        sym->mark = 1; // Mark as not in use
    }
    else
    {
        errorHandling(miscError);
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

    emit(SYS,0,HALT); // HALT
}

//<block> ::= <const-declaration> <var-declaration> <procedure-declaration> <statement>
void block ()
{
    int startCX = symCurr; // Store starting index of code for block before anything added to symbol table
    
    constDeclaration();
    int numVars = varDeclaration();

    int jmpIdx = cx;
    emit(JMP, 0, 0);

    // calling procedure declaration (part of hw4)
    procDeclaration();

    code[jmpIdx].m = cx * 3;
    emit (INC, 0, numVars + 3);
    statement();

    // Mark symbols
    for (int i = startCX; i < symCurr; i++)
    {
        if(symbolTable[i].kind == 2 || symbolTable[i].kind == 3)
        {
            symTableMark(&(symbolTable[i]));
        }
    }
}

//<procedure-declaration> ::= { "procedure" <indent> ";" <block> ";" }
void procDeclaration()
{
    // looping while current token is procedure
    while(tokens[pCurr] == procsym)
    {
        // incrementing and checking if current token is identifier
        pCurr++;
        if(tokens[pCurr] != identsym)
        {
            errorHandling(identifierExpected);
        }

        // saving name of procedure
        char nameOfIdent[12];
        strcpy(nameOfIdent, lexemes[pCurr]);

        // incrementing and checking if token is semicolon
        pCurr++;
        if(tokens[pCurr] != semicolonsym)
        {
            errorHandling(semicolonExpected);
        }

        // inserting procedure into symbol table
        pCurr++;
        symTableInsert(3, nameOfIdent, 0, currentLevel, cx);

        // incrementing/decrementing for procedure block
        currentLevel++;
        block();
        currentLevel--;

        // emit RTN to return to caller
        emit(OPR, 0, RET);

        // checking for semicolon
        if(tokens[pCurr] != semicolonsym)
        {
            errorHandling(semicolonExpected);
        }
        pCurr++;
    }
}

// <const-declaration> ::= [ "const" <ident> "=" <number> { "," <ident> "=" <number> } ";" ]
void constDeclaration ()
{
    // initializing for name and value of symbol
    char identName[12];
    int value = 0;

    // if it is a constant
    if(tokens[pCurr] == constsym)
    {   
        // loop at least once, continue while current token is a comma
        do
        {   
            // error check for identifier
            pCurr++;
            if(tokens[pCurr] != identsym)
            {
                errorHandling(identifierExpected);
            }

            // error check for duplicate symbol is in symTableInsert function
            
            // getting name of symbol
            strcpy(identName, lexemes[pCurr++]);

            // error check for certain symbol
            if(tokens[pCurr] != eqsym)
            {
                errorHandling(constantAssignmentSymbolExpected);
            }
            
            pCurr++;

            // error check for number
            if(tokens[pCurr] != numbersym)
            {
                errorHandling(numberExpected);
            }
            // getting number

            value = atoi(lexemes[pCurr]);

            // adding to symbol table
            symTableInsert(1, identName, value, 0, 0);
            pCurr++;

        } while (tokens[pCurr] == commasym);
        
        // error check for semicolon
        if(tokens[pCurr] != semicolonsym)
        {
            errorHandling(semicolonExpected);
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
                errorHandling(identifierExpected);
            }

            // symbol table duplicate check in symTableInsert function

            // adding to symbol table
            symTableInsert(2, lexemes[pCurr], 0, currentLevel, numVars + 3);
            numVars++;
            pCurr++;

        } while(tokens[pCurr] == commasym);
        // looping again if encountering comma

        // error check for semi colon
        if(tokens[pCurr] != semicolonsym)
        {
            errorHandling(semicolonExpected);
        }

        pCurr++;
    }
    return numVars;
}

/*
<statement> ::= [ <ident> ":=" <expression>
                | "call" <ident>
                | "begin" <statement> { ";" <statement> } "end"
                | "if" <condition> "then" <statement> [ "else" <statement> ] "fi"
                | "while" <condition> "do" <statement> "od"
                | "read" <ident>
                | "write" <expression> ]
*/
void statement ()
{
    // checking if is identifier
    if(tokens[pCurr] == identsym)
    {   
        // storing index of identifier
        int symIdx = symTableLookup(lexemes[pCurr]);

        // error check for undeclared identifier
        if(symIdx == -1)
        {
            errorHandling(undeclaredIdentifier);
        }

        // error check for non-variable
        if(symbolTable[symIdx].kind != 2)
        {
            errorHandling(cannotAlterNonVariable);
        }

        pCurr++;
        
        // error check for nonconstant identifier
        if(tokens[pCurr] != becomessym)
        {
            errorHandling(nonConstantIdentifierExpected);
        }

        pCurr++;

        // parsing and generating code from expression
        expression();
        
        // emitting STO
        emit(STO, getL(symIdx), symbolTable[symIdx].addr);
        return;
    }

    // checking if current token is callsym
    if (tokens[pCurr] == callsym)
    {
        pCurr++; //consume token
        int idx = symTableLookup(tokens[pCurr]); //get index

        // Handle errors
        if (tokens[pCurr] != identsym) //ensure token is an identifier
        {    
            errorHandling(identifierExpected);
        }

        if (idx == -1) // ensure token has been declared
        {
            errorHandling(undeclaredIdentifier);
        }

        if (symbolTable[idx].kind != 3) //ensure ident is a procedure
        {
            errorHandling(procedureExpected);
        }

        //emit instruction, consume token and return
        emit(CAL, getL(idx), symbolTable[idx].addr);
        pCurr++;
        return;
    }

    // checking if were at a beginning
    if(tokens[pCurr] == beginsym)
    {   
        // loop at least once, continue while current token is semi-colon
        do
        {
            pCurr++;
            // parsing and generating code from statement
            statement();
            
        } while(tokens[pCurr] == semicolonsym);

        // check if token is an ending, error if not
        if(tokens[pCurr] != endsym)
        {
            errorHandling(endExpected);
        }

        pCurr++;
        return;
    }

    // checking if current token is an if
    if(tokens[pCurr] == ifsym)
    {
        // incrementing and parsing and generating code for condition
        pCurr++;
        condition();

        // emitting JPC instruction 
        int jpcIdx = cx;
        emit(JPC, 0, 0); // M value to be updated later

        // error if does not encounter then
        if(tokens[pCurr] != thensym)
        {
            errorHandling(thenExpected);
        }
        pCurr++;

        // parsing statement and generating code, then filling in the jump address
        statement();

        int jmpIdx = -1;

        // detecting else statement
        if(tokens[pCurr] == elsesym)
        {
            pCurr++;
            jmpIdx = cx;
            emit(JMP, 0, 0);
            code[jpcIdx].m = cx;
            statement();
        }
        else
        {
            code[jpcIdx].m = cx;
        }


        if (tokens[pCurr] != fisym)
        {
            errorHandling(fiExpected); // Error 14: if-then statement must end with fi [cite: 169]
        }

        pCurr++; // Successfully consume 'fi'

        if(jmpIdx != -1)
        {
            code[jmpIdx].m = cx;
        }
        return;
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
            errorHandling(doExpected);
        }

        // incrementing token 
        pCurr++;

        // saving current index, and emitting JPC
        int jpcIdx = cx;
        emit(JPC, 0, 0);

        // parsing statement and generating code
        statement();

        if(tokens[pCurr] != odsym) 
        {
            errorHandling(odExpected); // Missing in your code
        }

        pCurr++; //consume odSym

        // emitting JMP and updating JPC instruction
        emit(JMP, 0, loopIdx);
        code[jpcIdx].m = cx;
        return;
    }

    // encountering read
    if(tokens[pCurr] == readsym)
    {   
        // incrementing token and error check for identifier
        pCurr++;
        if(tokens[pCurr] != identsym)
        {
            errorHandling(identifierExpected);
        }

        // getting index for identifier and error check if undeclared
        int symIdx = symTableLookup(lexemes[pCurr]);
        if(symIdx == -1)
        {
            errorHandling(undeclaredIdentifier);
        }

        // error check for unalterable variable
        if(symbolTable[symIdx].kind != 2)
        {
            errorHandling(cannotAlterNonVariable);
        }
        pCurr++;

        // emitting SYS and STO
        emit(SYS, 0, READ);
        emit(STO, getL(symIdx), symbolTable[symIdx].addr);
        return;
    }

    // encountering write
    if(tokens[pCurr] == writesym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting SYS
        emit(SYS, 0, WRITE);
        return ;
    }
}

// <condition> ::= <expression> <rel-op> <expression>
void condition ()
{   
    // parsing and generating code from expression
    expression();

    // encountering equal
    if(tokens[pCurr] == eqsym)
    {   
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting EQL
        emit(OPR, 0, EQL);
    }

    // encounter not equal
    else if(tokens[pCurr] == neqsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting NEQ
        emit(OPR, 0, NEQ);
    }

    // encountering less than
    else if(tokens[pCurr] == lessym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting LSS
        emit(OPR, 0, LSS);
    }

    // encounter less than or equal
    else if(tokens[pCurr] == leqsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting LEQ
        emit(OPR, 0, LEQ);
    }

    //  encounter greater than
    else if(tokens[pCurr] == gtrsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting GTR
        emit(OPR, 0, GTR);
    }

    // encounter greater than or equal
    else if(tokens[pCurr] == geqsym)
    {
        // incrementing token and parsing and generating code from expression
        pCurr++;
        expression();

        // emitting GEQ
        emit(OPR, 0, GEQ);
    }

    // error check for comparator expected
    else
    {
        errorHandling(comparatorExpected);
    }

}

// <expression> ::= ["-"] <term> { ("+" | "-") <term> }
void expression ()
{
    // checking for leading negative
    if(tokens[pCurr] == minussym)
    {
        pCurr++;
        term();
        emit(OPR, 0, NEG);
    }
    else
    {
        // parsing and generating code for term
        term();

        // loop while current token is plus or sym
        while(tokens[pCurr] == plussym || tokens[pCurr] == minussym)
        {
            // if plus
            if(tokens[pCurr] == plussym)
            {
                // incrementing token and parsing and generating code for term
                pCurr++;
                term();

                // emitting ADD
                emit(OPR, 0, ADD);
            }

            else
            {   
                // incrementing token and parsing and generate code for term
                pCurr++;
                term();

                // emitting SUB
                emit(OPR, 0, SUB);
            }
        }
    }
    
}

// <term> ::= <factor> { ("*" | "/" ) <factor> }
void term ()
{   
    // parsing and generating code for factor
    factor();

    // loop while current token is mult or div
    while(tokens[pCurr] == multsym || tokens[pCurr] == slashsym)
    {
        // if mult
        if(tokens[pCurr] == multsym)
        {
            // increment token and parse and generate code for factor
            pCurr++;
            factor();

            // emitting MUL
            emit(OPR, 0, MUL);
        }

        else
        {
            // increment token and parse and generate code for factor
            pCurr++;
            factor();

            // emitting DIV
            emit(OPR, 0, DIV);
        }
    }
}

// <factor> ::= <ident> | <number> | "(" <expression> ")"
void factor ()
{
    // encounter identifier
    if(tokens[pCurr] == identsym)
    {   
        // storing index of identifier
        int symIdx = symTableLookup(lexemes[pCurr]);
        if(symIdx == -1)
        {
            errorHandling(undeclaredIdentifier);
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
            emit(LOD, getL(symIdx), symbolTable[symIdx].addr);
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
        expression();

        // error check for right parenthesis expected
        if(tokens[pCurr] != rparentsym)
        {
            errorHandling(rightParenthesisExpected);
        }
        pCurr++;
    }

    // error check for arithmetic symbol expected
    else
    {
        errorHandling(arithmeticSymbolsExpected);
    }
}

// --- Code Generator helper function definitions ---

void emit (int op, int l, int m)
{
    if (cx < MAX)
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

// Calculates L value where index is the ident's index in the symTable
int getL (int index)
{
    int diff = 0; //const.L always = 0

    //if var or proc, do calculation
    if (symbolTable[index].kind == 2 || symbolTable[index].kind == 3)
        diff = currentLevel - symbolTable[index].level;

    return diff;
}

/*
    - Catches an errorcode thrown by other functions. 
    - Builds a message to print to stdout and elf.txt
    - Prints message to stdout and elf.txt then exits with error code
*/
void errorHandling (int errorCode)
{
    //Variable declaration
    char errorMessage [MAX] = "\n*\t--- ERROR: ";

    //Error Signaling
    printf("\n********************************");

    //Error detailing
    switch (errorCode)
    {
        case noError:    // Should never be called
        {
            strcat(errorMessage, " None! Programme ran successfully");
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

        case nonConstantIdentifierSymbolExpected:
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

        case symTableInsertionFailed:
        {
            strcat(errorMessage, "symbol table insertion failed");
            break;
        }

        case symTableLookupFailed:
        {
            strcat(errorMessage, "symbol table lookup failed");
            break;
        }
        
        case symTableMarkFailed:
        {
            strcat(errorMessage, "symbol table mark failed");
            break;
        }
        
        case nonConstantIdentifierExpected:
        {
            strcat(errorMessage, "only non-constant identifiers can be assigned a value");
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
    printf("%s",errorMessage);
    printf("\n********************************\n\n");
    fprintf(fOut, "%s", errorMessage);

    exit (errorCode);
}

// Gets name of OP code for printing to terminal
void getOpName (int op, char opName[4])
{
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
            errorHandling(miscError); 
            break;
        }
    }
}