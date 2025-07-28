#ifndef ASSEMBLE_H
#define ASSEMBLE_H

#include <stddef.h>
#include <stdint.h>

// Section types
typedef enum {
    SECTION_NONE = 0,
    SECTION_TEXT,
    SECTION_DATA
} SectionType;

// Operand types
typedef enum {
    OPERAND_NONE = 0,
    OPERAND_REGISTER,
    OPERAND_IMMEDIATE,
    OPERAND_LABEL,
    OPERAND_MEMORY
} OperandType;

// Operand
typedef struct {
    OperandType type;
    char value[64];
} Operand;

// Instruction
typedef struct Instruction {
    char mnemonic[16];
    Operand operands[2];
    SectionType section;
    int line_num;
    struct Instruction* next;
} Instruction;

// Label
typedef struct Label {
    char name[64];
    SectionType section;
    int address;
    struct Label* next;
} Label;

// Data definition
typedef struct DataDef {
    char directive[8];
    char value[64];
    int line_num;
    struct DataDef* next;
} DataDef;

// AST
typedef struct AST {
    Instruction* instructions;
    Label* labels;
    DataDef* data_defs;
} AST;

// Symbol table entry
typedef struct SymbolTableEntry {
    char name[64];
    SectionType section;
    int address;
    struct SymbolTableEntry* next;
} SymbolTableEntry;

// Symbol table
typedef struct {
    SymbolTableEntry* head;
} SymbolTable;

// Lexer token types and structures
typedef enum {
    TOKEN_LABEL,
    TOKEN_MNEMONIC,
    TOKEN_REGISTER,
    TOKEN_IMMEDIATE,
    TOKEN_SECTION,
    TOKEN_DIRECTIVE,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char text[64];
} Token;

typedef struct {
    const char *src;
    size_t pos;
} Lexer;

void lexer_init(Lexer *lexer, const char *src);
Token lexer_next_token(Lexer *lexer);

// Function prototypes
void add_instruction(AST* ast, Instruction* inst);
void add_label(AST* ast, Label* label);
void add_data_def(AST* ast, DataDef* def);
AST* parse(const char *src);
void build_symbol_table(AST* ast, SymbolTable* table);
int lookup_label(SymbolTable* table, const char* name, SectionType section);
int generate_code(AST *ast, SymbolTable *table, uint8_t **code, size_t *code_size, uint8_t **data, size_t *data_size);
int assemble_main(int argc, char *argv[]);
void symbol_table_init(SymbolTable* table);

#endif // ASSEMBLE_H 