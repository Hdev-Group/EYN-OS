#include <string.h>
#include <util.h>
#include <vga.h>
#include <fs_commands.h>
#include <eyn_exe_format.h>
#include <types.h>
#include <eynfs.h>
#include <assemble.h>
#include <shell_command_info.h>

void handler_assemble(string arg);
#define EYNFS_SUPERBLOCK_LBA 2048
extern uint8_t g_current_drive;

// --- Colored error/warning printing ---
void print_error(const char* file, int line, const char* msg, const char* line_text) {
    // Bright red: 255,0,0
    printf("\n");
    printf("[error] %s:%d: ", file, line);
    vga_set_color(255,0,0);
    printf("%s\n", msg);
    vga_set_color(255,255,255);
    if (line_text) {
        printf("    %s\n", line_text);
    }
}

void print_warning(const char* file, int line, const char* msg, const char* line_text) {
    // Pink: 255,105,180
    printf("\n");
    printf("[warning] %s:%d: ", file, line);
    vga_set_color(255,105,180);
    printf("%s\n", msg);
    vga_set_color(255,255,255);
    if (line_text) {
        printf("    %s\n", line_text);
    }
}

void lexer_init(Lexer *lexer, const char *src) {
    lexer->src = src;
    lexer->pos = 0;
}

Token lexer_next_token(Lexer *lexer) {
    Token token = {TOKEN_EOF, ""};
    const char* src = lexer->src;
    size_t len = strlen(src);
    size_t pos = lexer->pos;

    // Skip whitespace
    while (pos < len && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r')) pos++;

    // Skip comments
    if (pos < len && src[pos] == ';') {
        while (pos < len && src[pos] != '\n') pos++;
    }

    // Newline
    if (pos < len && src[pos] == '\n') {
        token.type = TOKEN_NEWLINE;
        token.text[0] = '\n';
        token.text[1] = 0;
        lexer->pos = pos + 1;
        return token;
    }

    // End of input
    if (pos >= len || src[pos] == 0) {
        token.type = TOKEN_EOF;
        token.text[0] = 0;
        lexer->pos = pos;
        return token;
    }

    // Comma
    if (src[pos] == ',') {
        token.type = TOKEN_COMMA;
        token.text[0] = ',';
        token.text[1] = 0;
        lexer->pos = pos + 1;
        return token;
    }

    // Identifiers, labels, mnemonics, registers, directives, section
    if ((src[pos] >= 'A' && src[pos] <= 'Z') || (src[pos] >= 'a' && src[pos] <= 'z') || src[pos] == '_' || src[pos] == '.') {
        size_t start = pos;
        while (pos < len && ((src[pos] >= 'A' && src[pos] <= 'Z') || (src[pos] >= 'a' && src[pos] <= 'z') || (src[pos] >= '0' && src[pos] <= '9') || src[pos] == '.' || src[pos] == '_')) pos++;
        size_t id_len = pos - start;
        if (id_len >= sizeof(token.text)) id_len = sizeof(token.text) - 1;
        memory_copy((char*)src + start, token.text, id_len);
        token.text[id_len] = 0;
        // Label (if followed by ':')
        if (src[pos] == ':') {
            token.type = TOKEN_LABEL;
            lexer->pos = pos + 1;
            return token;
        }
        // Section
        if (strcmp(token.text, "section") == 0) {
            token.type = TOKEN_SECTION;
            lexer->pos = pos;
            return token;
        }
        // Register - check against full register set
        if (is_valid_register(token.text)) {
            token.type = TOKEN_REGISTER;
            lexer->pos = pos;
            return token;
        }
        // Mnemonic - check against full instruction set
        if (is_valid_instruction(token.text)) {
            token.type = TOKEN_MNEMONIC;
            lexer->pos = pos;
            return token;
        }
        // Directive
        const char* dirs[] = {"db","dw","dd","global"};
        for (int i = 0; i < 4; i++) {
            if (strcmp(token.text, dirs[i]) == 0) {
                token.type = TOKEN_DIRECTIVE;
                lexer->pos = pos;
                return token;
            }
        }
        // Otherwise, treat as unknown identifier
        token.type = TOKEN_UNKNOWN;
        lexer->pos = pos;
        return token;
    }

    // Numbers (immediates)
    if (src[pos] >= '0' && src[pos] <= '9') {
        size_t start = pos;
        if (src[pos] == '0' && (src[pos+1] == 'x' || src[pos+1] == 'X')) {
            pos += 2;
            while (pos < len && ((src[pos] >= '0' && src[pos] <= '9') || (src[pos] >= 'a' && src[pos] <= 'f') || (src[pos] >= 'A' && src[pos] <= 'F'))) pos++;
        } else {
            while (pos < len && (src[pos] >= '0' && src[pos] <= '9')) pos++;
        }
        size_t num_len = pos - start;
        if (num_len >= sizeof(token.text)) num_len = sizeof(token.text) - 1;
        memory_copy((char*)src + start, token.text, num_len);
        token.text[num_len] = 0;
        token.type = TOKEN_IMMEDIATE;
        lexer->pos = pos;
        return token;
    }

    // Unknown/invalid
    token.type = TOKEN_UNKNOWN;
    token.text[0] = src[pos];
    token.text[1] = 0;
    lexer->pos = pos + 1;
    return token;
}

// --- AST appenders ---
void add_instruction(AST* ast, Instruction* inst) {
    inst->next = 0;
    if (!ast->instructions) {
        ast->instructions = inst;
    } else {
        Instruction* cur = ast->instructions;
        while (cur->next) cur = cur->next;
        cur->next = inst;
    }
}

void add_label(AST* ast, Label* label) {
    label->next = 0;
    if (!ast->labels) {
        ast->labels = label;
    } else {
        Label* cur = ast->labels;
        while (cur->next) cur = cur->next;
        cur->next = label;
    }
}

void add_data_def(AST* ast, DataDef* def) {
    def->next = 0;
    if (!ast->data_defs) {
        ast->data_defs = def;
    } else {
        DataDef* cur = ast->data_defs;
        while (cur->next) cur = cur->next;
        cur->next = def;
    }
}

// --- Code Generator ---
// Runner mapping assumptions (must match run_command.c):
// - Code is loaded at USER_CODE_ADDR
// - Data (if any) is placed at USER_CODE_ADDR + 0x1000
// We emit absolute addresses for labels by assuming code base = 0, data base = 0x1000 relative to code base.
// Must match run_command.c (USER_CODE_ADDR and data at code+0x1000)
static const int CODE_BASE = 0x200000;      // runtime code base
static const int DATA_BASE = 0x200000 + 0x1000; // runtime data base
// Helper: get register encoding (for mov/add)
static int reg_encoding(const char* reg) {
    return get_register_encoding(reg);
}

// Helper: parse immediate (decimal or hex)
static int parse_imm(const char* s) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        int val = 0;
        for (int i = 2; s[i]; i++) {
            char c = s[i];
            val *= 16;
            if (c >= '0' && c <= '9') val += c - '0';
            else if (c >= 'a' && c <= 'f') val += 10 + (c - 'a');
            else if (c >= 'A' && c <= 'F') val += 10 + (c - 'A');
        }
        return val;
    } else {
        return str_to_int((char*)s);
    }
}

// Helper function to check for buffer overflow
static int check_code_overflow(int code_pos, int buffer_size) {
    if (code_pos >= buffer_size) {
        printf("[codegen] Code buffer overflow! Generated %d bytes, but estimated limit was %d bytes.\n", code_pos, buffer_size);
        return 1;
    }
    return 0;
}

// Helper function to check for data buffer overflow
static int check_data_overflow(int data_pos, int buffer_size) {
    if (data_pos >= buffer_size) {
        printf("[codegen] Data buffer overflow! Generated %d bytes, but estimated limit was %d bytes.\n", data_pos, buffer_size);
        return 1;
    }
    return 0;
}

// Helper: build a simple label table with byte offsets
static int estimate_instr_size(const Instruction* inst) {
    if (!inst) return 1;
    if (strcmp(inst->mnemonic, "mov") == 0) {
        if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) return 5;
        if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_REGISTER) return 2;
        if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_LABEL) return 5;
        return 1;
    }
    if (!strcmp(inst->mnemonic, "add") || !strcmp(inst->mnemonic, "sub") || !strcmp(inst->mnemonic, "and") || !strcmp(inst->mnemonic, "or") || !strcmp(inst->mnemonic, "xor") || !strcmp(inst->mnemonic, "cmp")) {
        if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) return 6;
        return 1;
    }
    if (!strcmp(inst->mnemonic, "shl") || !strcmp(inst->mnemonic, "shr")) {
        if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) return 3;
        return 1;
    }
    if (!strcmp(inst->mnemonic, "jg")) return 6;
    if (!strcmp(inst->mnemonic, "jmp") || !strcmp(inst->mnemonic, "call")) return 5;
    if (!strcmp(inst->mnemonic, "ret")) return 1;
    if (!strcmp(inst->mnemonic, "int")) return 2;
    if (!strcmp(inst->mnemonic, "push")) { if (inst->operands[0].type == OPERAND_REGISTER) return 1; else return 5; }
    if (!strcmp(inst->mnemonic, "pop")) return 1;
    if (!strcmp(inst->mnemonic, "inc") || !strcmp(inst->mnemonic, "dec") || !strcmp(inst->mnemonic, "nop") || !strcmp(inst->mnemonic, "hlt") || !strcmp(inst->mnemonic, "cli") || !strcmp(inst->mnemonic, "sti")) return 1;
    return 1;
}

static void add_symbol(SymbolTable* table, const char* name, SectionType section, int address) {
    SymbolTableEntry* e = (SymbolTableEntry*)my_malloc(sizeof(SymbolTableEntry));
    if (!e) return;
    strncpy(e->name, name, sizeof(e->name)-1);
    e->name[sizeof(e->name)-1] = 0;
    e->section = section;
    e->address = address;
    e->next = table->head;
    table->head = e;
}

static void build_label_addresses(AST* ast, SymbolTable* table, int* out_text_size, int* out_data_size) {
    int text_bytes = 0;
    int data_bytes = 0;
    // First, estimate text size and assign label addresses tied to instruction indices
    Instruction* inst = ast->instructions;
    int inst_index = 0;
    while (inst) {
        if (inst->section == SECTION_TEXT) {
            // Assign any labels that point to this instruction index
            Label* lbl_it = ast->labels;
            while (lbl_it) {
                if (lbl_it->section == SECTION_TEXT && lbl_it->instr_index == inst_index) {
                    add_symbol(table, lbl_it->name, SECTION_TEXT, CODE_BASE + text_bytes);
                }
                lbl_it = lbl_it->next;
            }
            text_bytes += estimate_instr_size(inst);
            inst_index++;
        }
        inst = inst->next;
    }
    // Also assign labels that point exactly at end of text (if any)
    Label* lbl_tail = ast->labels;
    while (lbl_tail) {
        if (lbl_tail->section == SECTION_TEXT && lbl_tail->instr_index == inst_index) {
            add_symbol(table, lbl_tail->name, SECTION_TEXT, CODE_BASE + text_bytes);
        }
        lbl_tail = lbl_tail->next;
    }

    DataDef* def = ast->data_defs;
    int data_index = 0;
    while (def) {
        int added = 0;
        if (!strcmp(def->directive, "db")) added = 1; else if (!strcmp(def->directive, "dw")) added = 2; else if (!strcmp(def->directive, "dd")) added = 4;
        // Assign any labels that point to this data index
        Label* lbl2 = ast->labels;
        while (lbl2) {
            if (lbl2->section == SECTION_DATA && lbl2->instr_index == data_index) {
                add_symbol(table, lbl2->name, SECTION_DATA, DATA_BASE + data_bytes);
            }
            lbl2 = lbl2->next;
        }
        data_bytes += added;
        data_index++;
        def = def->next;
    }
    if (out_text_size) *out_text_size = text_bytes;
    if (out_data_size) *out_data_size = data_bytes;
}

// Main code generator
// For now, only .text section is emitted as code, .data as data
// Returns 0 on success, sets *code, *code_size, *data, *data_size
int generate_code(AST *ast, SymbolTable *table, uint8_t **code, size_t *code_size, uint8_t **data, size_t *data_size, const char* input_path) {
    // First pass: count actual sizes needed
    int text_bytes = 0;
    int data_bytes = 0;
    // First pass: compute label addresses and better estimates
    build_label_addresses(ast, table, &text_bytes, &data_bytes);
    Instruction* inst = ast->instructions;
    while (inst) {
        if (inst->section == SECTION_TEXT) {
            // Estimate instruction size based on type
            const InstructionInfo* info = find_instruction_info(inst->mnemonic);
            if (info) {
                text_bytes += 1; // opcode
                if (info->modrm_required) text_bytes += 1; // ModR/M
                if (info->immediate_required) text_bytes += 4; // immediate
                if (info->displacement_required) text_bytes += 4; // displacement
            } else {
                text_bytes += 4; // fallback
            }
        }
        inst = inst->next;
    }
    DataDef* def = ast->data_defs;
    while (def) {
        if (strcmp(def->directive, "db") == 0) data_bytes += 1;
        else if (strcmp(def->directive, "dw") == 0) data_bytes += 2;
        else if (strcmp(def->directive, "dd") == 0) data_bytes += 4;
        def = def->next;
    }
    
    // Add some padding for safety
    text_bytes += 64;
    data_bytes += 64;
    
    // Cap at reasonable limits (16KB each)
    if (text_bytes > 16384) text_bytes = 16384;
    if (data_bytes > 16384) data_bytes = 16384;
    
    // Memory safety: limit total allocation to prevent heap exhaustion
    size_t total_allocation = text_bytes + data_bytes;
    if (total_allocation > 8192) { // 8KB limit for assembler
        printf("[codegen] Warning: Total allocation too large (%d bytes), limiting to 8KB\n", total_allocation);
        if (text_bytes > 4096) text_bytes = 4096;
        if (data_bytes > 4096) data_bytes = 4096;
        // Ensure total doesn't exceed 8KB
        if (text_bytes + data_bytes > 8192) {
            text_bytes = 4096;
            data_bytes = 4096;
        }
    }
    
    printf("[codegen] Estimated sizes - Code: %d bytes, Data: %d bytes\n", text_bytes, data_bytes);
    
    *code = (uint8_t*)my_malloc(text_bytes);
    *data = (uint8_t*)my_malloc(data_bytes);
    if (!*code || !*data) {
        printf("[codegen] Out of memory for code/data buffers\n");
        if (*code) my_free(*code);
        if (*data) my_free(*data);
        *code = NULL;
        *data = NULL;
        return 1;
    }
    *code_size = text_bytes;
    *data_size = data_bytes;
    
    // Emit code
    int code_pos = 0;
    int actual_code_size = 0;
    inst = ast->instructions;
    while (inst) {
        if (inst->section != SECTION_TEXT) { inst = inst->next; continue; }
        
        printf("[codegen] Emitting %s at %d\n", inst->mnemonic, code_pos);
        
        const InstructionInfo* info = find_instruction_info(inst->mnemonic);
        if (!info) {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown instruction: %s", inst->mnemonic);
            print_error(input_path, inst->line_num, msg, NULL);
            (*code)[code_pos++] = 0x90; // NOP as fallback
            inst = inst->next;
            continue;
        }
        
        // Emit opcode
        (*code)[code_pos++] = info->opcode;
        
            // Check for buffer overflow after opcode
    if (check_code_overflow(code_pos, text_bytes)) {
        return 1;
    }
        
        // Handle different instruction types
        if (strcmp(inst->mnemonic, "mov") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && 
                inst->operands[1].type == OPERAND_IMMEDIATE) {
                // mov reg, imm32
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0xB8 + reg; // Override opcode
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    
                    // Check for buffer overflow after mov reg, imm32
                    if (check_code_overflow(code_pos, text_bytes)) {
                        return 1;
                    }
                    
                    printf("[codegen] mov %s, %d\n", inst->operands[0].value, imm);
                }
            } else if (inst->operands[0].type == OPERAND_REGISTER && 
                       inst->operands[1].type == OPERAND_REGISTER) {
                // mov reg, reg
                int reg1 = reg_encoding(inst->operands[0].value);
                int reg2 = reg_encoding(inst->operands[1].value);
                if (reg1 >= 0 && reg2 >= 0) {
                    (*code)[code_pos-1] = 0x88; // mov r/m, r
                    (*code)[code_pos++] = 0xC0 | (reg2 << 3) | reg1; // ModR/M
                    
                    // Check for buffer overflow after mov reg, reg
                    if (check_code_overflow(code_pos, text_bytes)) {
                        return 1;
                    }
                    
                    printf("[codegen] mov %s, %s\n", inst->operands[0].value, inst->operands[1].value);
                }
            } else if (inst->operands[0].type == OPERAND_REGISTER &&
                       inst->operands[1].type == OPERAND_LABEL) {
                // mov reg, label -> absolute address
                int reg = reg_encoding(inst->operands[0].value);
                int addr = lookup_label(table, inst->operands[1].value, SECTION_TEXT);
                if (addr < 0) addr = lookup_label(table, inst->operands[1].value, SECTION_DATA);
                if (addr < 0) {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Undefined label: %s", inst->operands[1].value);
                    print_error(input_path, inst->line_num, msg, NULL);
                    addr = 0;
                }
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0xB8 + reg;
                    (*code)[code_pos++] = addr & 0xFF;
                    (*code)[code_pos++] = (addr >> 8) & 0xFF;
                    (*code)[code_pos++] = (addr >> 16) & 0xFF;
                    (*code)[code_pos++] = (addr >> 24) & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "add") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && 
                inst->operands[1].type == OPERAND_IMMEDIATE) {
                // add reg, imm32
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x81; // add r/m, imm32
                    (*code)[code_pos++] = 0xC0 | reg; // ModR/M
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    
                    if (check_code_overflow(code_pos, text_bytes)) {
                        return 1;
                    }
                    
                    printf("[codegen] add %s, %d\n", inst->operands[0].value, imm);
                }
            }
        } else if (strcmp(inst->mnemonic, "sub") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && 
                inst->operands[1].type == OPERAND_IMMEDIATE) {
                // sub reg, imm32
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x81; // sub r/m, imm32
                    (*code)[code_pos++] = 0xE8 | reg; // ModR/M
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    
                    if (check_code_overflow(code_pos, text_bytes)) {
                        return 1;
                    }
                    
                    printf("[codegen] sub %s, %d\n", inst->operands[0].value, imm);
                }
            }
        } else if (strcmp(inst->mnemonic, "jmp") == 0) {
            if (inst->operands[0].type == OPERAND_LABEL) {
                // jmp rel32
                int target = lookup_label(table, inst->operands[0].value, SECTION_TEXT);
                int rel = (target >= 0) ? (target - (code_pos + 5)) : 0;
                (*code)[code_pos-1] = 0xE9; // jmp rel32
                (*code)[code_pos++] = rel & 0xFF;
                (*code)[code_pos++] = (rel >> 8) & 0xFF;
                (*code)[code_pos++] = (rel >> 16) & 0xFF;
                (*code)[code_pos++] = (rel >> 24) & 0xFF;
                
                if (check_code_overflow(code_pos, text_bytes)) {
                    return 1;
                }
                
                printf("[codegen] jmp %s (rel %d)\n", inst->operands[0].value, rel);
            }
        } else if (strcmp(inst->mnemonic, "call") == 0) {
            if (inst->operands[0].type == OPERAND_LABEL) {
                // call rel32
                int target = lookup_label(table, inst->operands[0].value, SECTION_TEXT);
                int rel = (target >= 0) ? (target - (code_pos + 5)) : 0;
                (*code)[code_pos-1] = 0xE8; // call rel32
                (*code)[code_pos++] = rel & 0xFF;
                (*code)[code_pos++] = (rel >> 8) & 0xFF;
                (*code)[code_pos++] = (rel >> 16) & 0xFF;
                (*code)[code_pos++] = (rel >> 24) & 0xFF;
                
                if (check_code_overflow(code_pos, text_bytes)) {
                    return 1;
                }
                
                printf("[codegen] call %s (rel %d)\n", inst->operands[0].value, rel);
            }
        } else if (strcmp(inst->mnemonic, "ret") == 0) {
            (*code)[code_pos-1] = 0xC3; // ret
            printf("[codegen] ret\n");
        } else if (strcmp(inst->mnemonic, "int") == 0) {
            if (inst->operands[0].type == OPERAND_IMMEDIATE) {
                int imm = parse_imm(inst->operands[0].value);
                (*code)[code_pos-1] = 0xCD; // int imm8
                (*code)[code_pos++] = imm & 0xFF;
                
                if (check_code_overflow(code_pos, text_bytes)) {
                    return 1;
                }
                
                printf("[codegen] int %d\n", imm);
            }
        } else if (strcmp(inst->mnemonic, "push") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER) {
                int reg = reg_encoding(inst->operands[0].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x50 + reg; // push reg
                    printf("[codegen] push %s\n", inst->operands[0].value);
                }
            } else if (inst->operands[0].type == OPERAND_IMMEDIATE) {
                int imm = parse_imm(inst->operands[0].value);
                (*code)[code_pos-1] = 0x68; // push imm32
                (*code)[code_pos++] = imm & 0xFF;
                (*code)[code_pos++] = (imm >> 8) & 0xFF;
                (*code)[code_pos++] = (imm >> 16) & 0xFF;
                (*code)[code_pos++] = (imm >> 24) & 0xFF;
                
                if (check_code_overflow(code_pos, text_bytes)) {
                    return 1;
                }
                
                printf("[codegen] push %d\n", imm);
            }
        } else if (strcmp(inst->mnemonic, "pop") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER) {
                int reg = reg_encoding(inst->operands[0].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x58 + reg; // pop reg
                    printf("[codegen] pop %s\n", inst->operands[0].value);
                }
            }
        } else if (strcmp(inst->mnemonic, "inc") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER) {
                int reg = reg_encoding(inst->operands[0].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x40 + reg; // inc reg
                    printf("[codegen] inc %s\n", inst->operands[0].value);
                }
            }
        } else if (strcmp(inst->mnemonic, "dec") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER) {
                int reg = reg_encoding(inst->operands[0].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x48 + reg; // dec reg
                    printf("[codegen] dec %s\n", inst->operands[0].value);
                }
            }
        } else if (strcmp(inst->mnemonic, "nop") == 0) {
            (*code)[code_pos-1] = 0x90; // nop
            printf("[codegen] nop\n");
        } else if (strcmp(inst->mnemonic, "hlt") == 0) {
            (*code)[code_pos-1] = 0xF4; // hlt
            printf("[codegen] hlt\n");
        } else if (strcmp(inst->mnemonic, "cli") == 0) {
            (*code)[code_pos-1] = 0xFA; // cli
            printf("[codegen] cli\n");
        } else if (strcmp(inst->mnemonic, "sti") == 0) {
            (*code)[code_pos-1] = 0xFB; // sti
            printf("[codegen] sti\n");
        } else if (strcmp(inst->mnemonic, "and") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) {
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x81; // and r/m, imm32
                    (*code)[code_pos++] = 0xE0 | reg; // ModR/M for AND
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "or") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) {
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x81; // or r/m, imm32
                    (*code)[code_pos++] = 0xC8 | reg; // ModR/M for OR
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "xor") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) {
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x81; // xor r/m, imm32
                    (*code)[code_pos++] = 0xF0 | reg; // ModR/M for XOR
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "shl") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) {
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0xC1; // shl r/m, imm8
                    (*code)[code_pos++] = 0xE0 | reg; // ModR/M for SHL
                    (*code)[code_pos++] = imm & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "shr") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) {
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0xC1; // shr r/m, imm8
                    (*code)[code_pos++] = 0xE8 | reg; // ModR/M for SHR
                    (*code)[code_pos++] = imm & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "cmp") == 0) {
            if (inst->operands[0].type == OPERAND_REGISTER && inst->operands[1].type == OPERAND_IMMEDIATE) {
                int reg = reg_encoding(inst->operands[0].value);
                int imm = parse_imm(inst->operands[1].value);
                if (reg >= 0) {
                    (*code)[code_pos-1] = 0x81; // cmp r/m, imm32
                    (*code)[code_pos++] = 0xF8 | reg; // ModR/M for CMP
                    (*code)[code_pos++] = imm & 0xFF;
                    (*code)[code_pos++] = (imm >> 8) & 0xFF;
                    (*code)[code_pos++] = (imm >> 16) & 0xFF;
                    (*code)[code_pos++] = (imm >> 24) & 0xFF;
                    if (check_code_overflow(code_pos, text_bytes)) return 1;
                }
            }
        } else if (strcmp(inst->mnemonic, "jg") == 0) {
            if (inst->operands[0].type == OPERAND_LABEL) {
                int target = lookup_label(table, inst->operands[0].value, SECTION_TEXT);
                int rel = (target >= 0) ? (target - (code_pos + 6)) : 0; // rel32
                (*code)[code_pos-1] = 0x0F; // 2-byte opcode prefix
                (*code)[code_pos++] = 0x8F; // jg rel32
                (*code)[code_pos++] = rel & 0xFF;
                (*code)[code_pos++] = (rel >> 8) & 0xFF;
                (*code)[code_pos++] = (rel >> 16) & 0xFF;
                (*code)[code_pos++] = (rel >> 24) & 0xFF;
                if (check_code_overflow(code_pos, text_bytes)) return 1;
            }
        } else {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unsupported instruction: %s", inst->mnemonic);
            print_error(input_path, inst->line_num, msg, NULL);
            (*code)[code_pos-1] = 0x90; // NOP as fallback
        }
        
        inst = inst->next;
        actual_code_size = code_pos;
    }
    
    // Emit data
    int data_pos = 0;
    int actual_data_size = 0;
    def = ast->data_defs;
    while (def) {
        // Check for buffer overflow
        if (check_data_overflow(data_pos, data_bytes)) {
            return 1;
        }
        
        printf("[codegen] Emitting data %s %s at %d\n", def->directive, def->value, data_pos);
        int val = parse_imm(def->value);
        if (strcmp(def->directive, "db") == 0) {
            (*data)[data_pos++] = val & 0xFF;
        } else if (strcmp(def->directive, "dw") == 0) {
            (*data)[data_pos++] = val & 0xFF;
            (*data)[data_pos++] = (val >> 8) & 0xFF;
        } else if (strcmp(def->directive, "dd") == 0) {
            (*data)[data_pos++] = val & 0xFF;
            (*data)[data_pos++] = (val >> 8) & 0xFF;
            (*data)[data_pos++] = (val >> 16) & 0xFF;
            (*data)[data_pos++] = (val >> 24) & 0xFF;
        }
        def = def->next;
        actual_data_size = data_pos;
    }
    // Override sizes to actual emitted sizes
    *code_size = (size_t)actual_code_size;
    *data_size = (size_t)actual_data_size;
    printf("[codegen] Code size: %d, Data size: %d\n", actual_code_size, actual_data_size);
    return 0;
}

// --- File I/O helpers ---
// Reads the entire file at 'filename' from the current drive into a buffer allocated with my_malloc.
// Returns pointer and sets out_size, or NULL on error.
char* read_file_to_buffer(const char* filename, uint32_t* out_size) {
    printf("[assemble] read_file_to_buffer: filename='%s'\n", filename);
    printf("[assemble] g_current_drive = %d\n", g_current_drive);
    eynfs_superblock_t sb;
    printf("[assemble] Reading superblock at LBA %d...\n", EYNFS_SUPERBLOCK_LBA);
    int sb_res = eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb);
    printf("[assemble] eynfs_read_superblock returned %d\n", sb_res);
    if (sb_res != 0) {
        printf("[assemble] No supported filesystem found (read error).\n");
        return 0;
    }
    printf("[assemble] Superblock magic: 0x%X (expected 0x%X)\n", sb.magic, EYNFS_MAGIC);
    if (sb.magic != EYNFS_MAGIC) {
        printf("[assemble] No supported filesystem found (bad magic).\n");
        return 0;
    }
    eynfs_dir_entry_t entry;
    if (eynfs_find_in_dir(g_current_drive, &sb, sb.root_dir_block, filename, &entry, 0) != 0) {
        printf("[assemble] File not found: %s\n", filename);
        return 0;
    }
    uint32_t size = entry.size;
    
    // Memory safety: limit file size to prevent excessive allocation
    if (size > 8192) { // 8KB limit for source files
        printf("[assemble] Warning: Source file too large (%d bytes), limiting to 8KB\n", size);
        size = 8192;
    }
    
    char* buf = (char*)my_malloc(size + 1);
    if (!buf) {
        printf("[assemble] Out of memory.\n");
        return 0;
    }
    int n = eynfs_read_file(g_current_drive, &sb, &entry, buf, size, 0);
    if (n < 0) {
        printf("[assemble] Failed to read file: %s\n", filename);
        my_free(buf);
        return 0;
    }
    buf[size] = '\0';
    if (out_size) *out_size = size;
    return buf;
}

int assemble(const char *input_path, const char *output_path) {
    uint32_t src_size = 0;
    char* src = read_file_to_buffer(input_path, &src_size);
    if (!src) {
        print_error(input_path, 0, "Failed to read input file or out of memory.", NULL);
        return 1;
    }

    AST *ast = parse(src);
    SymbolTable symtab;
    symbol_table_init(&symtab);
    uint8_t *code = 0;
    size_t code_size = 0;
    uint8_t *data = 0;
    size_t data_size = 0;
    int gen_result = generate_code(ast, &symtab, &code, &code_size, &data, &data_size, input_path);
    if (gen_result != 0) {
        print_error(input_path, 0, "Code generation failed.", NULL);
        my_free(src);
        return 1;
    }

    // Prepare EYN-OS header
    struct eyn_exe_header hdr;
    memory_set((uint8*)&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, EYN_MAGIC, 4);
    hdr.version = EYN_EXE_VERSION;
    hdr.flags = 0;  // No special flags for now
    hdr.reserved = 0;  // Reserved field
    // Look up the _start label to get the entry point
    int start_addr = lookup_label(&symtab, "_start", SECTION_TEXT);
    if (start_addr >= 0) {
        hdr.entry_point = start_addr - CODE_BASE;  // Convert absolute address to offset
    } else {
        hdr.entry_point = 0;  // Default to start of code if no _start label
    }
    hdr.code_size = code_size;
    hdr.data_size = data_size;
    hdr.bss_size = 0;  // No BSS for now
    hdr.dyn_table_off = 0;  // No dynamic linking for now
    hdr.dyn_table_size = 0;  // No dynamic linking for now

    // Write header + code to output file
    // Allocate output buffer
    size_t outbuf_size = sizeof(hdr) + code_size + data_size;
    
    // Memory safety: limit output buffer size
    if (outbuf_size > 16384) { // 16KB limit for output buffer
        printf("[assemble] Warning: Output buffer too large (%d bytes), limiting to 16KB\n", outbuf_size);
        outbuf_size = 16384;
    }
    
    char* outbuf = (char*)my_malloc(outbuf_size);
    if (!outbuf) {
        print_error(input_path, 0, "Out of memory while allocating output buffer.", NULL);
        my_free(src);
        return 1;
    }
    memory_copy((char*)&hdr, outbuf, sizeof(hdr));
    if (code && code_size > 0) {
        // Bounds check to prevent buffer overflow
        if (sizeof(hdr) + code_size <= outbuf_size) {
            memory_copy((char*)code, outbuf + sizeof(hdr), code_size);
        } else {
            printf("[assemble] Warning: Code size (%d) exceeds buffer bounds\n", code_size);
        }
        my_free(code);
    }
    if (data && data_size > 0) {
        // Bounds check to prevent buffer overflow
        if (sizeof(hdr) + code_size + data_size <= outbuf_size) {
            memory_copy((char*)data, outbuf + sizeof(hdr) + code_size, data_size);
        } else {
            printf("[assemble] Warning: Data size (%d) exceeds buffer bounds\n", data_size);
        }
        my_free(data);
    }
    int res = write_output_to_file(outbuf, outbuf_size, output_path, g_current_drive);
    if (res == 0) {
        printf("Assembled and wrote %s successfully.\n", output_path);
    } else {
        printf("Failed to write output file: %s\n", output_path);
        printf("This may indicate filesystem corruption. Try rebooting.\n");
    }
    my_free(outbuf);
    my_free(src);
    free_ast(ast);  // Free the AST instead of the TODO comment
    return res;
}

int assemble_main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: assemble <input_asm_file> <output_bin_file>\n");
        return 1;
    }
    return assemble(argv[1], argv[2]);
} 

// Ensure these functions are not static and are implemented:

void symbol_table_init(SymbolTable* table) {
    table->head = 0;
}

int lookup_label(SymbolTable* table, const char* name, SectionType section) {
    SymbolTableEntry* cur = table->head;
    while (cur) {
        if (cur->section == section && strcmp(cur->name, name) == 0) {
            return cur->address;
        }
        cur = cur->next;
    }
    return -1;
}

// Helper to check if a label is used (stub: always returns 0 for now)
int label_is_used(const char* name, SymbolTable* table) {
    // TODO: Implement real usage tracking
    return 0;
}

AST* parse(const char *src) {
    Lexer lexer;
    lexer_init(&lexer, src);
    Token token;
    SectionType current_section = SECTION_NONE;
    int line_num = 1;
    AST* ast = (AST*)my_malloc(sizeof(AST));
    if (!ast) {
        printf("[parse] Out of memory for AST\n");
        return 0;
    }
    ast->instructions = 0;
    ast->labels = 0;
    ast->data_defs = 0;
    int text_instr_index = 0;
    int data_def_index = 0;

    while (1) {
        token = lexer_next_token(&lexer);
        if (token.type == TOKEN_EOF) break;
        if (token.type == TOKEN_NEWLINE) {
            line_num++;
            continue;
        }
        if (token.type == TOKEN_SECTION) {
            // Expect .text or .data next
            Token next = lexer_next_token(&lexer);
            if (strcmp(next.text, ".text") == 0) {
                current_section = SECTION_TEXT;
                printf("[parse] Section: .text\n");
            } else if (strcmp(next.text, ".data") == 0) {
                current_section = SECTION_DATA;
                printf("[parse] Section: .data\n");
            } else {
                printf("[parse] Unknown section: %s\n", next.text);
                current_section = SECTION_NONE;
            }
            continue;
        }
        if (token.type == TOKEN_LABEL) {
            // Add label to AST
            Label* label = (Label*)my_malloc(sizeof(Label));
            if (!label) continue;
            strncpy(label->name, token.text, sizeof(label->name)-1);
            label->name[sizeof(label->name)-1] = 0;
            label->section = current_section;
            label->address = 0; // To be filled in codegen
            label->line_num = line_num;
            // Record position within section for later address calc
            if (current_section == SECTION_TEXT) {
                label->instr_index = text_instr_index;
            } else if (current_section == SECTION_DATA) {
                label->instr_index = data_def_index;
            } else {
                label->instr_index = 0;
            }
            add_label(ast, label);
            printf("[parse] Label: %s (section %d)\n", label->name, label->section);
            continue;
        }
        if (token.type == TOKEN_MNEMONIC) {
            // Parse instruction and operands
            Instruction* inst = (Instruction*)my_malloc(sizeof(Instruction));
            if (!inst) continue;
            strncpy(inst->mnemonic, token.text, sizeof(inst->mnemonic)-1);
            inst->mnemonic[sizeof(inst->mnemonic)-1] = 0;
            inst->section = current_section;
            inst->line_num = line_num;
            // Parse up to 2 operands with comma handling
            int op_index = 0;
            while (op_index < 2) {
                Token next = lexer_next_token(&lexer);
                if (next.type == TOKEN_COMMA) {
                    // stray comma; skip
                    continue;
                } else if (next.type == TOKEN_REGISTER) {
                    inst->operands[op_index].type = OPERAND_REGISTER;
                    strncpy(inst->operands[op_index].value, next.text, sizeof(inst->operands[op_index].value)-1);
                    inst->operands[op_index].value[sizeof(inst->operands[op_index].value)-1] = 0;
                    op_index++;
                    // expect optional comma after operand, consume if present
                    Token maybe_comma = lexer_next_token(&lexer);
                    if (maybe_comma.type != TOKEN_COMMA) {
                        // push back not supported; just treat as next token in stream by rewinding pos one token length is complex
                        // no-op: our lexer has no pushback, so rely on whitespace/line end
                    }
                } else if (next.type == TOKEN_IMMEDIATE) {
                    inst->operands[op_index].type = OPERAND_IMMEDIATE;
                    strncpy(inst->operands[op_index].value, next.text, sizeof(inst->operands[op_index].value)-1);
                    inst->operands[op_index].value[sizeof(inst->operands[op_index].value)-1] = 0;
                    op_index++;
                    Token maybe_comma = lexer_next_token(&lexer);
                    if (maybe_comma.type != TOKEN_COMMA) {
                        // see above
                    }
                } else if (next.type == TOKEN_LABEL || next.type == TOKEN_UNKNOWN) {
                    inst->operands[op_index].type = OPERAND_LABEL;
                    strncpy(inst->operands[op_index].value, next.text, sizeof(inst->operands[op_index].value)-1);
                    inst->operands[op_index].value[sizeof(inst->operands[op_index].value)-1] = 0;
                    op_index++;
                    Token maybe_comma = lexer_next_token(&lexer);
                    if (maybe_comma.type != TOKEN_COMMA) {
                    }
                } else {
                    break;
                }
            }
            add_instruction(ast, inst);
            if (current_section == SECTION_TEXT) {
                text_instr_index++;
            }
            printf("[parse] Instruction: %s (section %d, line %d)\n", inst->mnemonic, inst->section, inst->line_num);
            continue;
        }
        if (token.type == TOKEN_DIRECTIVE) {
            // Data definition or global
            if (strcmp(token.text, "global") == 0) {
                // Skip for now (could add to export table)
                Token next = lexer_next_token(&lexer);
                printf("[parse] Global: %s\n", next.text);
                continue;
            } else {
                // db, dw, dd
                DataDef* def = (DataDef*)my_malloc(sizeof(DataDef));
                if (!def) continue;
                strncpy(def->directive, token.text, sizeof(def->directive)-1);
                def->directive[sizeof(def->directive)-1] = 0;
                // Parse value (stub: just grab next token)
                Token next = lexer_next_token(&lexer);
                strncpy(def->value, next.text, sizeof(def->value)-1);
                def->value[sizeof(def->value)-1] = 0;
                def->line_num = line_num;
                add_data_def(ast, def);
                if (current_section == SECTION_DATA) {
                    data_def_index++;
                }
                printf("[parse] Data: %s %s (line %d)\n", def->directive, def->value, def->line_num);
                continue;
            }
        }
        // Unknown or unsupported token: skip
    }
    return ast;
}

REGISTER_SHELL_COMMAND(assemble, "assemble", handler_assemble, CMD_STREAMING, "Converts assembly code into machine code.\nSupports NASM syntax.\nUsage: assemble <input file> <output file>", "assemble example.asm example.eyn");

// Free AST and all its components
void free_ast(AST* ast) {
    if (!ast) return;
    
    // Free instruction linked list
    Instruction* inst = ast->instructions;
    while (inst) {
        Instruction* next = inst->next;
        my_free(inst);
        inst = next;
    }
    
    // Free label linked list
    Label* label = ast->labels;
    while (label) {
        Label* next = label->next;
        my_free(label);
        label = next;
    }
    
    // Free data definition linked list
    DataDef* data_def = ast->data_defs;
    while (data_def) {
        DataDef* next = data_def->next;
        my_free(data_def);
        data_def = next;
    }
    
    // Free the AST itself
    my_free(ast);
}