#include "../../../include/string.h"
#include "../../../include/util.h"
#include "../../../include/vga.h"
#include "../../../include/fs_commands.h"
#include "../../../include/eyn_exe_format.h"
#include "../../../include/types.h"
#include "../../../include/eynfs.h"
#include "../../../include/assemble.h"
#include "../../../include/shell_command_info.h"
#define EYNFS_SUPERBLOCK_LBA 2048
extern uint8_t g_current_drive;

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
        // Register
        const char* regs[] = {"eax","ebx","ecx","edx","esi","edi","esp","ebp"};
        for (int i = 0; i < 8; i++) {
            if (strcmp(token.text, regs[i]) == 0) {
                token.type = TOKEN_REGISTER;
                lexer->pos = pos;
                return token;
            }
        }
        // Mnemonic
        const char* mnems[] = {"mov","add","sub","jmp","call","ret","int"};
        for (int i = 0; i < 7; i++) {
            if (strcmp(token.text, mnems[i]) == 0) {
                token.type = TOKEN_MNEMONIC;
                lexer->pos = pos;
                return token;
            }
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
// Helper: get register encoding (for mov/add)
static int reg_encoding(const char* reg) {
    if (strcmp(reg, "eax") == 0) return 0;
    if (strcmp(reg, "ecx") == 0) return 1;
    if (strcmp(reg, "edx") == 0) return 2;
    if (strcmp(reg, "ebx") == 0) return 3;
    if (strcmp(reg, "esp") == 0) return 4;
    if (strcmp(reg, "ebp") == 0) return 5;
    if (strcmp(reg, "esi") == 0) return 6;
    if (strcmp(reg, "edi") == 0) return 7;
    return -1;
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

// Main code generator
// For now, only .text section is emitted as code, .data as data
// Returns 0 on success, sets *code, *code_size, *data, *data_size
int generate_code(AST *ast, SymbolTable *table, uint8_t **code, size_t *code_size, uint8_t **data, size_t *data_size) {
    // Count instructions and data size
    int text_bytes = 0;
    int data_bytes = 0;
    Instruction* inst = ast->instructions;
    while (inst) {
        if (inst->section == SECTION_TEXT) {
            // mov/add: 5 bytes, jmp: 5 bytes, ret: 1 byte (max)
            if (strcmp(inst->mnemonic, "mov") == 0 || strcmp(inst->mnemonic, "add") == 0) text_bytes += 5;
            else if (strcmp(inst->mnemonic, "jmp") == 0) text_bytes += 5;
            else if (strcmp(inst->mnemonic, "ret") == 0) text_bytes += 1;
            else text_bytes += 4; // fallback
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
    *code = (uint8_t*)my_malloc(text_bytes);
    *data = (uint8_t*)my_malloc(data_bytes);
    if (!*code || !*data) {
        printf("[codegen] Out of memory for code/data buffers\n");
        return 1;
    }
    *code_size = text_bytes;
    *data_size = data_bytes;
    // Emit code
    int code_pos = 0;
    inst = ast->instructions;
    while (inst) {
        if (inst->section != SECTION_TEXT) { inst = inst->next; continue; }
        printf("[codegen] Emitting %s at %d\n", inst->mnemonic, code_pos);
        if (strcmp(inst->mnemonic, "mov") == 0 &&
            inst->operands[0].type == OPERAND_REGISTER &&
            inst->operands[1].type == OPERAND_IMMEDIATE) {
            int reg = reg_encoding(inst->operands[0].value);
            int imm = parse_imm(inst->operands[1].value);
            if (reg >= 0) {
                (*code)[code_pos++] = 0xB8 + reg; // mov reg, imm32
                (*code)[code_pos++] = imm & 0xFF;
                (*code)[code_pos++] = (imm >> 8) & 0xFF;
                (*code)[code_pos++] = (imm >> 16) & 0xFF;
                (*code)[code_pos++] = (imm >> 24) & 0xFF;
                printf("[codegen] mov %s, %d\n", inst->operands[0].value, imm);
            } else {
                printf("[codegen] Unknown register: %s\n", inst->operands[0].value);
            }
        } else if (strcmp(inst->mnemonic, "add") == 0 &&
                   inst->operands[0].type == OPERAND_REGISTER &&
                   inst->operands[1].type == OPERAND_IMMEDIATE) {
            int reg = reg_encoding(inst->operands[0].value);
            int imm = parse_imm(inst->operands[1].value);
            if (reg >= 0) {
                (*code)[code_pos++] = 0x81;
                (*code)[code_pos++] = 0xC0 + reg; // add reg, imm32
                (*code)[code_pos++] = imm & 0xFF;
                (*code)[code_pos++] = (imm >> 8) & 0xFF;
                (*code)[code_pos++] = (imm >> 16) & 0xFF;
                (*code)[code_pos++] = (imm >> 24) & 0xFF;
                printf("[codegen] add %s, %d\n", inst->operands[0].value, imm);
            } else {
                printf("[codegen] Unknown register: %s\n", inst->operands[0].value);
            }
        } else if (strcmp(inst->mnemonic, "jmp") == 0 &&
                   inst->operands[0].type == OPERAND_LABEL) {
            // jmp rel32: E9 xx xx xx xx
            int target = lookup_label(table, inst->operands[0].value, SECTION_TEXT);
            int rel = (target >= 0) ? (target - (code_pos + 5)) : 0;
            (*code)[code_pos++] = 0xE9;
            (*code)[code_pos++] = rel & 0xFF;
            (*code)[code_pos++] = (rel >> 8) & 0xFF;
            (*code)[code_pos++] = (rel >> 16) & 0xFF;
            (*code)[code_pos++] = (rel >> 24) & 0xFF;
            printf("[codegen] jmp %s (rel %d)\n", inst->operands[0].value, rel);
        } else if (strcmp(inst->mnemonic, "ret") == 0) {
            (*code)[code_pos++] = 0xC3;
            printf("[codegen] ret\n");
        } else {
            printf("[codegen] Unsupported instruction: %s\n", inst->mnemonic);
            // Emit NOP (90h) as fallback
            (*code)[code_pos++] = 0x90;
        }
        inst = inst->next;
    }
    // Emit data
    int data_pos = 0;
    def = ast->data_defs;
    while (def) {
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
    }
    printf("[codegen] Code size: %d, Data size: %d\n", (int)*code_size, (int)*data_size);
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
    if (!src) return 1;

    AST *ast = parse(src);
    SymbolTable symtab;
    symbol_table_init(&symtab);
    uint8_t *code = 0;
    size_t code_size = 0;
    uint8_t *data = 0;
    size_t data_size = 0;
    generate_code(ast, &symtab, &code, &code_size, &data, &data_size);

    // Prepare EYN-OS header
    struct eyn_exe_header hdr;
    memory_set((uint8*)&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, EYN_MAGIC, 4);
    hdr.version = EYN_EXE_VERSION;
    hdr.code_size = code_size;
    hdr.data_size = data_size;
    // TODO: Set other header fields

    // Write header + code to output file
    // Allocate output buffer
    size_t outbuf_size = sizeof(hdr) + code_size + data_size;
    char* outbuf = (char*)my_malloc(outbuf_size);
    if (!outbuf) {
        printf("Out of memory.\n");
        my_free(src);
        return 1;
    }
    memory_copy((char*)&hdr, outbuf, sizeof(hdr));
    if (code && code_size > 0) {
        memory_copy((char*)code, outbuf + sizeof(hdr), code_size);
        my_free(code);
    }
    if (data && data_size > 0) {
        memory_copy((char*)data, outbuf + sizeof(hdr) + code_size, data_size);
        my_free(data);
    }
    int res = write_output_to_file(outbuf, outbuf_size, output_path, g_current_drive);
    if (res == 0) {
        printf("Assembled and wrote %s successfully.\n", output_path);
    } else {
        printf("Failed to write output file: %s\n", output_path);
    }
    my_free(outbuf);
    my_free(src);
    // TODO: Free AST
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

AST* parse(const char *src) {
    Lexer lexer;
    lexer_init(&lexer, src);
    Token token;
    SectionType current_section = SECTION_NONE;
    int line_num = 1;
    AST* ast = (AST*)my_malloc(sizeof(AST));
    if (!ast) return 0;
    ast->instructions = 0;
    ast->labels = 0;
    ast->data_defs = 0;

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
            // Parse up to 2 operands
            for (int i = 0; i < 2; i++) {
                Token next = lexer_next_token(&lexer);
                if (next.type == TOKEN_REGISTER) {
                    inst->operands[i].type = OPERAND_REGISTER;
                    strncpy(inst->operands[i].value, next.text, sizeof(inst->operands[i].value)-1);
                    inst->operands[i].value[sizeof(inst->operands[i].value)-1] = 0;
                } else if (next.type == TOKEN_IMMEDIATE) {
                    inst->operands[i].type = OPERAND_IMMEDIATE;
                    strncpy(inst->operands[i].value, next.text, sizeof(inst->operands[i].value)-1);
                    inst->operands[i].value[sizeof(inst->operands[i].value)-1] = 0;
                } else if (next.type == TOKEN_LABEL || next.type == TOKEN_UNKNOWN) {
                    inst->operands[i].type = OPERAND_LABEL;
                    strncpy(inst->operands[i].value, next.text, sizeof(inst->operands[i].value)-1);
                    inst->operands[i].value[sizeof(inst->operands[i].value)-1] = 0;
                } else {
                    inst->operands[i].type = OPERAND_NONE;
                    inst->operands[i].value[0] = 0;
                }
                // If next is comma, skip it
                if (next.type == TOKEN_COMMA) continue;
                // If not a valid operand, break
                if (next.type != TOKEN_REGISTER && next.type != TOKEN_IMMEDIATE && next.type != TOKEN_LABEL && next.type != TOKEN_UNKNOWN) break;
                // If only one operand, break
                if (i == 0 && (next.type != TOKEN_COMMA)) break;
            }
            add_instruction(ast, inst);
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
                printf("[parse] Data: %s %s (line %d)\n", def->directive, def->value, def->line_num);
                continue;
            }
        }
        // Unknown or unsupported token: skip
    }
    return ast;
}

REGISTER_SHELL_COMMAND(assemble, "assemble", "Converts assembly code into machine code.\nSupports NASM syntax.\nUsage: assemble <input file> <output file>", "assemble example.asm example.eyn");