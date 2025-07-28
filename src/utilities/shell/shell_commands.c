#include "../../../include/shell_commands.h"
#include "../../../include/shell_command_info.h"
#include "../../../include/fs_commands.h"
#include "../../../include/types.h"
#include "../../../include/vga.h"
#include "../../../include/util.h"
#include "../../../include/math.h"
#include "../../../include/system.h"
#include "../../../include/string.h"
#include "../../../include/eynfs.h"
#include "../../../include/shell.h"
#include "../../../include/game_engine.h"
#include <stdint.h>

#define EYNFS_SUPERBLOCK_LBA 2048
extern uint8_t g_current_drive;

// Random number generator command
void random_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    // If no arguments, generate a single random number
    if (!ch[i]) {
        uint32_t num = rand_next();
        printf("%cRandom number: %d\n", 255, 255, 255, (int)num);
        return;
    }
    
    // Parse first argument (count or min) - using safe parsing like drive_cmd
    if (ch[i] >= '0' && ch[i] <= '9') {
        uint32_t arg1 = 0;
        while (ch[i] >= '0' && ch[i] <= '9') {
            arg1 = arg1 * 10 + (ch[i] - '0');
            i++;
        }
        
        // Skip spaces
        while (ch[i] && ch[i] == ' ') i++;
        
        // Check if there's a second argument
        if (ch[i] && ch[i] >= '0' && ch[i] <= '9') {
            uint32_t arg2 = 0;
            while (ch[i] >= '0' && ch[i] <= '9') {
                arg2 = arg2 * 10 + (ch[i] - '0');
                i++;
            }
            
            // Two arguments: range [min, max]
            if (arg1 >= arg2) {
                printf("%cError: min must be less than max\n", 255, 0, 0);
                return;
            }
            
            uint32_t num = rand_range(arg1, arg2);
            printf("%cRandom number in range [%d, %d]: %d\n", 255, 255, 255, (int)arg1, (int)arg2, (int)num);
        } else {
            printf("%cGenerating %d random numbers:\n", 255, 255, 255, (int)arg1);
            for (uint32_t k = 0; k < arg1; k++) {
                uint32_t num = rand_next();
                printf("%c%d", 255, 255, 255, (int)num);
                if (k < arg1 - 1) printf(", ");
                if ((k + 1) % 10 == 0) printf("\n");
            }
            printf("\n");
        }
    } else {
        printf("%cError: Invalid number format\n", 255, 0, 0);
        return;
    }
}

// history command implementation
void history_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        // Show history
        show_history(&g_command_history);
    } else {
        // Check for clear command
        char arg[16] = {0};
        uint8 j = 0;
        while (ch[i] && ch[i] != ' ' && j < 15) arg[j++] = ch[i++];
        arg[j] = '\0';
        
        if (strcmp(arg, "clear") == 0) {
            clear_history(&g_command_history);
            printf("%cCommand history cleared.\n", 0, 255, 0);
        } else {
            printf("%cUsage: history [clear]\n", 255, 255, 255);
            printf("%c  history      - Show command history\n", 255, 255, 255);
            printf("%c  history clear - Clear command history\n", 255, 255, 255);
        }
    }
}

// sort command implementation
void sort_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: sort <string1> <string2> <string3> ...\n", 255, 255, 255);
        printf("%cExample: sort zebra apple banana\n", 255, 255, 255);
        return;
    }
    
    // Count the number of strings
    int count = 0;
    int pos = i;
    while (ch[pos]) {
        if (ch[pos] == ' ') {
            count++;
            while (ch[pos] == ' ') pos++;
        } else {
            pos++;
        }
    }
    count++; // Count the last string
    
    if (count == 0) {
        printf("%cNo strings to sort.\n", 255, 0, 0);
        return;
    }
    
    // Allocate array of string pointers
    char** strings = (char**) my_malloc(count * sizeof(char*));
    if (!strings) {
        printf("%cError: Memory allocation failed.\n", 255, 0, 0);
        return;
    }
    
    // Parse strings
    pos = i;
    int str_idx = 0;
    while (ch[pos] && str_idx < count) {
        // Skip leading spaces
        while (ch[pos] == ' ') pos++;
        if (!ch[pos]) break;
        
        // Find end of current string
        int start = pos;
        while (ch[pos] && ch[pos] != ' ') pos++;
        int len = pos - start;
        
        // Allocate and copy string
        strings[str_idx] = (char*) my_malloc(len + 1);
        if (!strings[str_idx]) {
            printf("%cError: Memory allocation failed.\n", 255, 0, 0);
            // Clean up
            for (int j = 0; j < str_idx; j++) {
                my_free(strings[j]);
            }
            my_free(strings);
            return;
        }
        
        // Copy string
        for (int j = 0; j < len; j++) {
            strings[str_idx][j] = ch[start + j];
        }
        strings[str_idx][len] = '\0';
        str_idx++;
    }
    
    // Sort the strings
    if (count > 1) {
        quicksort_strings(strings, 0, count - 1);
    }
    
    // Print sorted strings
    for (int j = 0; j < count; j++) {
        printf("%c%d: %s\n", 255, 255, 255, j + 1, strings[j]);
        my_free(strings[j]);
    }
    
    my_free(strings);
}

// Recursive search function
void search_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                     const char* pattern, int search_filenames, int search_contents, 
                     int* found_count, char* current_path, int path_len) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0) return;
    
    for (int k = 0; k < count; k++) {
        eynfs_dir_entry_t* entry = &entries[k];
        
        if (entry->name[0] == '\0') continue;
        

        
        // Build full path for this entry
        char full_path[256];
        if (strcmp(current_path, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", entry->name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entry->name);
        }
        
        int match_found = 0;
        
        // Search in filename/directory name
        if (search_filenames) {
            if (boyer_moore_search(entry->name, pattern) != -1) {
                if (entry->type == EYNFS_TYPE_DIR) {
                    printf("%c[DIRNAME] %s/\n", 0, 255, 0, full_path);
                } else {
                    printf("%c[FILENAME] %s\n", 0, 255, 0, full_path);
                }
                match_found = 1;
                (*found_count)++;
            }
        }
        
        // Search in file contents (only for files)
        if (search_contents && entry->type == EYNFS_TYPE_FILE && !match_found) {
            // Read file content
            uint8_t* file_content = (uint8_t*)my_malloc(entry->size + 1);
            if (file_content) {
                int bytes_read = eynfs_read_file(drive, sb, entry, file_content, entry->size, 0);
                if (bytes_read > 0) {
                    file_content[bytes_read] = '\0'; // Null-terminate for string search
                    
                    if (boyer_moore_search((char*)file_content, pattern) != -1) {
                        printf("%c[CONTENT] %s\n", 255, 255, 0, full_path);
                        (*found_count)++;
                    }
                }
                my_free(file_content);
            }
        }
        
        // Recursively search subdirectories
        if (entry->type == EYNFS_TYPE_DIR) {
            search_recursive(drive, sb, entry->first_block, pattern, 
                           search_filenames, search_contents, found_count, 
                           full_path, strlen(full_path));
        }
    }
}

// search command implementation
void search_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: search <pattern> [options]\n", 255, 255, 255);
        printf("%cOptions:\n", 255, 255, 255);
        printf("%c  -f    Search in filenames and directory names only\n", 255, 255, 255);
        printf("%c  -c    Search in file contents only (default)\n", 255, 255, 255);
        printf("%c  -a    Search in both names and contents\n", 255, 255, 255);
        printf("%cExample: search hello -a\n", 255, 255, 255);
        return;
    }
    
    // Parse search pattern
    char pattern[64] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) {
        pattern[j++] = ch[i++];
    }
    pattern[j] = '\0';
    
    if (strlen(pattern) == 0) {
        printf("%cError: No search pattern provided.\n", 255, 0, 0);
        return;
    }
    
    // Parse options
    int search_filenames = 1; // Default to searching names too
    int search_contents = 1; // Default to content search
    
    while (ch[i] && ch[i] == ' ') i++;
    if (ch[i] == '-') {
        i++;
        if (ch[i] == 'f') {
            search_filenames = 1;
            search_contents = 0;
        } else if (ch[i] == 'c') {
            search_filenames = 0;
            search_contents = 1;
        } else if (ch[i] == 'a') {
            search_filenames = 1;
            search_contents = 1;
        }
    }
    
    printf("%cSearching for '%s' in entire filesystem...\n", 255, 255, 255, pattern);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    // Start recursive search from root
    int found_count = 0;
    search_recursive(g_current_drive, &sb, sb.root_dir_block, pattern, 
                    search_filenames, search_contents, &found_count, "/", 1);
    
    if (found_count == 0) {
        printf("%cNo matches found for '%s'.\n", 255, 0, 0, pattern);
    } else {
        printf("%cFound %d match(es) for '%s'.\n", 0, 255, 0, found_count, pattern);
    }
}

// game command implementation
void game_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: game <filename>\n", 255, 255, 255);
        printf("%cExample: game snake\n", 255, 255, 255);
        printf("%cLoads and runs a game from a .txt file.\n", 255, 255, 255);
        return;
    }
    
    // Parse filename
    char filename[64] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) {
        filename[j++] = ch[i++];
    }
    filename[j] = '\0';
    
    if (strlen(filename) == 0) {
        printf("%cError: No game filename provided.\n", 255, 0, 0);
        return;
    }
    
    // Add .txt extension if not present
    if (strlen(filename) < 4 || strcmp(filename + strlen(filename) - 4, ".txt") != 0) {
        strncat(filename, ".txt", sizeof(filename) - strlen(filename) - 1);
    }
    
    // Resolve the path properly
    char abspath[128];
    resolve_path(filename, shell_current_path, abspath, sizeof(abspath));
    
    printf("%cLoading game: %s\n", 255, 255, 255, abspath);
    
    // Initialize game engine
    if (game_engine_init() != 0) {
        printf("%cError: Failed to initialize game engine.\n", 255, 0, 0);
        return;
    }
    
    // Load and run game
    game_state_t state;
    if (game_load_from_text_file(abspath, &state) != 0) {
        printf("%cError: Failed to load game file.\n", 255, 0, 0);
        game_engine_cleanup();
        return;
    }
    
    // Run the game
    game_run(&state);
    
    // Cleanup
    game_free_state(&state);
    game_engine_cleanup();
}

// echo implementation
void echo(string ch)
{
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%c\n", 255, 255, 255);
        return;
    }
    printf("%c%s\n", 255, 255, 255, &ch[i]);
}

// joke_spam implementation
void joke_spam() 
{
    for (int i = 1; i <= 100; i++) 
    {
        printf("%c EYN-OS\n", 255, 0, 255);
    }
    printf("%c\n", 255, 255, 255);
}

// ver implementation
void ver() 
{
    printf("%c#######  ##    ##  ###     ##          ######    #####\n", 255, 0, 255);
    printf("%c###       ##  ##   ####    ##         ##    ##  ##\n");
    printf("%c#######     ##     ##  ##  ##  #####  ##    ##   #####\n");
    printf("%c###         ##     ##    ####         ##    ##       ##\n");
    printf("%c#######     ##     ##      ##          ######    #####\n");
    printf("%c(Release 11)\n", 200, 200, 200);
}

// help implementation
void help()
{
    extern const shell_command_info_t __start_shellcmds[];
    extern const shell_command_info_t __stop_shellcmds[];
    
    // Count commands
    int cmd_count = 0;
    for (const shell_command_info_t* cmd = __start_shellcmds; cmd < __stop_shellcmds; ++cmd) {
        cmd_count++;
    }
    
    if (cmd_count == 0) {
        printf("%cNo commands available.\n", 255, 255, 255);
        return;
    }
    
    // Create array of command pointers for sorting
    const shell_command_info_t** cmd_array = (const shell_command_info_t**) my_malloc(cmd_count * sizeof(const shell_command_info_t*));
    if (!cmd_array) {
        printf("%cError: Memory allocation failed.\n", 255, 0, 0);
        return;
    }
    
    // Fill array with command pointers
    int idx = 0;
    for (const shell_command_info_t* cmd = __start_shellcmds; cmd < __stop_shellcmds; ++cmd) {
        cmd_array[idx++] = cmd;
    }
    
    // Sort commands alphabetically by name
    if (cmd_count > 1) {
        // Simple bubble sort for command structures
        for (int i = 0; i < cmd_count - 1; i++) {
            for (int j = 0; j < cmd_count - i - 1; j++) {
                if (strcmp(cmd_array[j]->name, cmd_array[j + 1]->name) > 0) {
                    const shell_command_info_t* temp = cmd_array[j];
                    cmd_array[j] = cmd_array[j + 1];
                    cmd_array[j + 1] = temp;
                }
            }
        }
    }
    
    // Display sorted commands
    for (int i = 0; i < cmd_count; i++) {
        const shell_command_info_t* cmd = cmd_array[i];
        printf("%c%-12s: %s\n", 255, 255, 255, cmd->name, cmd->description);
        if (cmd->example && cmd->example[0])
            printf("  Example: %s\n", cmd->example);
    }
    
    my_free(cmd_array);
}

REGISTER_SHELL_COMMAND(help, "help", "Display this message and show all available commands with descriptions and examples.\nUsage: help", "help");
REGISTER_SHELL_COMMAND(echo, "echo", "Reprints a given text to the screen.\nUsage: echo <text>", "echo Hello, world!");
REGISTER_SHELL_COMMAND(ver, "ver", "Shows the current system version and release information.\nUsage: ver", "ver");
REGISTER_SHELL_COMMAND(spam, "spam", "Spam 'EYN-OS' to the shell 100 times for fun.\nUsage: spam", "spam");
REGISTER_SHELL_COMMAND(calc, "calc", "32-bit fixed-point calculator. Supports +, -, *, /.\nUsage: calc <expression>", "calc 2.5+3.7");
REGISTER_SHELL_COMMAND(draw, "draw", "Draw a rectangle.\nUsage: draw <x> <y> <width> <height> <r> <g> <b>.\nExample: draw 10 20 100 50 255 0 0 draws a red rectangle.", "draw 10 20 100 50 255 0 0");
REGISTER_SHELL_COMMAND(drive, "drive", "Change between different drives (from lsata).\nUsage: drive <n>", "drive 0");
REGISTER_SHELL_COMMAND(memory, "memory", "Memory management and testing.\nUsage: memory stats | test | stress", "memory stats");
REGISTER_SHELL_COMMAND(log, "log", "Enable or disable shell logging.\nUsage: log on|off", "log on");
REGISTER_SHELL_COMMAND(lsata, "lsata", "List detected ATA drives and their details.\nUsage: lsata", "lsata");
REGISTER_SHELL_COMMAND(exit, "exit", "Exits the kernel and shuts down the system.\nUsage: exit", "exit");
REGISTER_SHELL_COMMAND(clear, "clear", "Clears the screen and resets the shell display.\nUsage: clear", "clear");
REGISTER_SHELL_COMMAND(catram, "catram", "Display contents of a file from RAM disk (FAT32).\nUsage: catram <filename>", "catram test.txt");
REGISTER_SHELL_COMMAND(lsram, "lsram", "List files in the RAM disk (FAT32) with directory tree.\nUsage: lsram", "lsram");
REGISTER_SHELL_COMMAND(random, "random", "Generate random numbers.\nUsage: random [count] | random [min] [max]\nExample: random 5 | random 10 20", "random 5");
REGISTER_SHELL_COMMAND(sort, "sort", "Sort strings alphabetically.\nUsage: sort <string1> <string2> <string3> ...\nExample: sort zebra apple banana", "sort zebra apple banana");
REGISTER_SHELL_COMMAND(search, "search", "Search for text in filenames and file contents using Boyer-Moore algorithm.\nUsage: search <pattern> [-f|-c|-a]\nExample: search hello -a", "search hello -a");
REGISTER_SHELL_COMMAND(game, "game", "Load and run a game from a .dat file.\nUsage: game <filename>\nExample: game snake", "game snake");

// draw_cmd_handler implementation
void draw_cmd_handler(string ch) 
{
    int x = 10, y = 10, w = 500, h = 200, r = 255, g = 255, b = 255;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) 
    {
        printf("%cUsage: draw <x> <y> <width> <height> <r> <g> <b>\n", 255, 255, 255);
        printf("%cExample: draw 10 20 100 50 255 0 0\n");
        printf("%cIf you provide fewer than 7 parameters, defaults will be used for the rest.\n");
        return;
    }
    char argstr[128];
    uint8 j = 0;
    while (ch[i] && j < 127) 
    {
        argstr[j++] = ch[i++];
    }
    argstr[j] = '\0';
    int vals[7] = {x, y, w, h, r, g, b};
    int val_idx = 0;
    char numbuf[16];
    int ni = 0;
    for (uint8 k = 0; argstr[k] && val_idx < 7; k++) 
    {
        if ((argstr[k] >= '0' && argstr[k] <= '9') || argstr[k] == '-') 
        {
            numbuf[ni++] = argstr[k];
        } else if (argstr[k] == ' ' || argstr[k] == '\t') 
        {
            if (ni > 0) 
            {
                numbuf[ni] = '\0';
                vals[val_idx++] = str_to_int(numbuf);
                ni = 0;
            }
        }
    }
    if (ni > 0 && val_idx < 7) 
    {
        numbuf[ni] = '\0';
        vals[val_idx++] = str_to_int(numbuf);
    }
    x = vals[0]; y = vals[1]; w = vals[2]; h = vals[3]; r = vals[4]; g = vals[5]; b = vals[6];
    drawRect(x, y, w, h, r, g, b);
    printf("%cShape drawn.\n", 255, 255, 255);
}

// calc implementation
void calc(string str) 
{
    uint8 i = 0;
    while (str[i] && str[i] != ' ') i++;
    while (str[i] && str[i] == ' ') i++;
    if (!str[i]) 
    {
        printf("%cUsage: calc <expression>\n", 255, 255, 255);
        printf("%cExample: calc 2.5+3.7\n");
        return;
    }
    char expression[200];
    uint8 j = 0;
    while (str[i] && j < 199) {
        expression[j++] = str[i++];
    }
    expression[j] = '\0';
    int32_t result = math_get_current_equation(expression);
    int32_t int_part = result / FIXED_POINT_FACTOR;
    int32_t decimal_part = result % FIXED_POINT_FACTOR;
    if (decimal_part < 0) decimal_part = -decimal_part;
    if (decimal_part == 0) 
    {
        printf("%cResult: %d\n", 255, 255, 255, int_part);
    } 
    else 
    {
        char decimal_str[4];
        decimal_str[0] = '0' + (decimal_part / 100);
        decimal_str[1] = '0' + ((decimal_part / 10) % 10);
        decimal_str[2] = '0' + (decimal_part % 10);
        decimal_str[3] = '\0';
        printf("%cResult: %d.%s\n", 255, 255, 255, int_part, decimal_str);
    }
}

// lsata implementation
void lsata() {
    printf("%cDetected Drives:\n", 255, 255, 255);
    printf("%c================\n", 255, 255, 255);
    
    int found_drives = 0;
    for (int d = 0; d < 8; d++) {
        uint16 id[256];
        int res = ata_identify(d, id);
        if (res == 0) {
            found_drives++;
            char model[41];
            for (int i = 0; i < 20; i++) {
                model[i*2] = (id[27+i] >> 8) & 0xFF;
                model[i*2+1] = id[27+i] & 0xFF;
            }
            model[40] = '\0';
            
            // Clean up model name (remove trailing spaces)
            int len = strlen(model);
            while (len > 0 && model[len-1] == ' ') {
                model[len-1] = '\0';
                len--;
            }
            
            uint32 sectors = id[60] | (id[61] << 16);
            uint32 mb = (sectors / 2048);
            uint32 gb = mb / 1024;
            
            // Determine drive type
            const char* drive_type = "IDE";
            if (id[83] & 0x0400) {
                drive_type = "SATA";
            }
            
            printf("%cDrive %d: %s\n", 255, 255, 255, d, model);
            printf("%c  Type: %s\n", 255, 255, 255, drive_type);
            printf("%c  Size: %d MB (%d GB)\n", 255, 255, 255, mb, gb);
            printf("%c  Sectors: %d\n", 255, 255, 255, sectors);
            printf("%c  Status: Present and responding\n", 0, 255, 0);
            printf("\n");
        } else {
            printf("%cDrive %d: Not present or not responding\n", 255, 165, 0, d);
        }
    }
    
    if (found_drives == 0) {
        printf("%cNo drives detected. This may indicate:\n", 255, 0, 0);
        printf("%c- SATA drives in AHCI mode (not legacy IDE mode)\n", 255, 0, 0);
        printf("%c- Drive controller not properly initialized\n", 255, 0, 0);
        printf("%c- Hardware compatibility issues\n", 255, 0, 0);
        printf("%c\nTry running 'fdisk' to check for MBR/partition table\n", 255, 255, 255);
    } else {
        printf("%cTotal drives found: %d\n", 0, 255, 0, found_drives);
    }
}

// drives command implementation - detailed drive diagnostics
void drives_cmd(string ch) {
    printf("%cDrive Diagnostics and Troubleshooting\n", 255, 255, 255);
    printf("%c====================================\n", 255, 255, 255);
    
    // Check if any drives are detected
    int detected_count = 0;
    for (int d = 0; d < 8; d++) {
        if (ata_drive_present(d)) {
            detected_count++;
        }
    }
    
    printf("%cDetected drives: %d\n", 255, 255, 255, detected_count);
    
    if (detected_count == 0) {
        printf("%c\nTroubleshooting Steps:\n", 255, 255, 0);
        printf("%c1. Check BIOS settings - ensure SATA is in 'Legacy IDE' mode\n", 255, 255, 255);
        printf("%c2. Verify drive power and data cables are connected\n", 255, 255, 255);
        printf("%c3. Try different SATA ports on motherboard\n", 255, 255, 255);
        printf("%c4. Check if drive appears in BIOS boot menu\n", 255, 255, 255);
        printf("%c5. Try running 'fdisk' to check for MBR\n", 255, 255, 255);
        printf("%c\nCommon Dell Optiplex 755 Issues:\n", 255, 255, 0);
        printf("%c- SATA drives may be in AHCI mode by default\n", 255, 255, 255);
        printf("%c- BIOS may need 'Compatibility' or 'Legacy' mode\n", 255, 255, 255);
        printf("%c- Some drives may require jumper settings\n", 255, 255, 255);
    } else {
        printf("%c\nDrive Information:\n", 255, 255, 0);
        for (int d = 0; d < 8; d++) {
            if (ata_drive_present(d)) {
                printf("%cDrive %d: Present\n", 0, 255, 0, d);
            }
        }
    }
    
    printf("%c\nCommands to try:\n", 255, 255, 0);
    printf("%c- 'lsata' - List all detected drives\n", 255, 255, 255);
    printf("%c- 'fdisk' - Check partition table\n", 255, 255, 255);
    printf("%c- 'format 0 eynfs' - Format drive 0 with EYNFS\n", 255, 255, 255);
    printf("%c- 'drive 0' - Switch to drive 0\n", 255, 255, 255);
}

// drive_cmd implementation
void drive_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (ch[i] >= '0' && ch[i] <= '9') {
        uint32_t drive = 0;
        while (ch[i] >= '0' && ch[i] <= '9') {
            drive = drive * 10 + (ch[i] - '0');
            i++;
        }
        g_current_drive = (uint8_t)drive;
        printf("%cSwitched to drive %d\n", 0, 255, 0, g_current_drive);
    } else {
        printf("%cUsage: drive <n>\n", 255, 255, 255);
    }
}

// memory_cmd implementation
void memory_cmd(string ch) {
    printf("%cMemory Management Commands:\n", 255, 255, 255);
    printf("%c  memory stats    - Show memory statistics\n", 255, 255, 255);
    printf("%c  memory test     - Run memory allocation test\n", 255, 255, 255);
    printf("%c  memory stress   - Run stress test\n", 255, 255, 255);
    char* space = strchr(ch, ' ');
    if (space) {
        space++;
        if (strcmp(space, "stats") == 0) {
            print_memory_stats();
        }
        else if (strcmp(space, "test") == 0) {
            printf("%cRunning memory allocation test...\n", 255, 255, 255);
            void* ptr1 = my_malloc(100);
            void* ptr2 = my_malloc(200);
            void* ptr3 = my_malloc(50);
            if (ptr1 && ptr2 && ptr3) {
                printf("%cBasic allocation test: PASSED\n", 0, 255, 0);
                my_free(ptr2);
                printf("%cFree test: PASSED\n", 0, 255, 0);
                void* new_ptr = my_realloc(ptr1, 150);
                if (new_ptr) {
                    printf("%cRealloc test: PASSED\n", 0, 255, 0);
                    my_free(new_ptr);
                }
                my_free(ptr3);
            } else {
                printf("%cBasic allocation test: FAILED\n", 255, 0, 0);
            }
            print_memory_stats();
        }
        else if (strcmp(space, "stress") == 0) {
            printf("%cRunning memory stress test...\n", 255, 255, 255);
            void* ptrs[100];
            int count = 0;
            for (int i = 0; i < 100; i++) {
                ptrs[i] = my_malloc(16 + (i % 50));
                if (ptrs[i]) count++;
            }
            printf("%cAllocated %d blocks\n", 255, 255, 255, count);
            for (int i = 0; i < 100; i += 2) {
                if (ptrs[i]) my_free(ptrs[i]);
            }
            printf("%cFreed every other block\n", 255, 255, 255);
            for (int i = 0; i < 50; i++) {
                ptrs[i] = my_malloc(32 + (i % 100));
            }
            printf("%cAllocated more blocks\n", 255, 255, 255);
            for (int i = 0; i < 100; i++) {
                if (ptrs[i]) my_free(ptrs[i]);
            }
            printf("%cStress test completed\n", 0, 255, 0);
            print_memory_stats();
        }
    }
}

// size implementation
void size(string ch) {
    uint8 disk = g_current_drive;
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    if (!ch[i]) {
        printf("%cUsage: size <filename>\n", 255, 255, 255);
        return;
    }
    char arg[64];
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) arg[j++] = ch[i++];
    arg[j] = '\0';
    if (strlength(arg) < 1) {
        printf("%cUsage: size <filename>\n", 255, 255, 255);
        return;
    }
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(disk, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        uint32_t entry_idx;
        if (eynfs_find_in_dir(disk, &sb, sb.root_dir_block, arg, &entry, &entry_idx) != 0 || entry.type != EYNFS_TYPE_FILE) {
            printf("%cFile not found in EYNFS root directory.\n", 255, 0, 0);
            return;
        }
        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf), "%s: %u bytes", arg, entry.size);
        printf("%c%s\n", 255, 255, 255, outbuf);
        return;
    }
    printf("%cNo supported filesystem found or file not found.\n", 255, 0, 0);
} 

void log_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    char arg[8] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 7) arg[j++] = ch[i++];
    arg[j] = '\0';
    if (strEql(arg, "on")) {
        shell_log_enable();
        printf("%clogging enabled\n", 0, 255, 0);
    } else if (strEql(arg, "off")) {
        shell_log_disable();
        printf("%clogging disabled\n", 255, 0, 0);
    } else {
        printf("%cUsage: log on|off\n", 255, 255, 255);
    }
} 

// Hexdump command: prints up to 64 bytes of a file in hex
void hexdump_cmd(string ch) {
    char filename[64] = {0};
    int i = 0, j = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    while (ch[i] && ch[i] != ' ' && j < 63) filename[j++] = ch[i++];
    filename[j] = '\0';
    if (!filename[0]) {
        printf("Usage: hexdump <file>\n");
        return;
    }
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(0, 2048, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("[hexdump] Failed to read superblock\n");
        return;
    }
    eynfs_dir_entry_t entry;
    uint32_t entry_idx;
    if (eynfs_find_in_dir(0, &sb, sb.root_dir_block, filename, &entry, &entry_idx) != 0) {
        printf("[hexdump] File not found: %s\n", filename);
        return;
    }
    uint8_t buf[64];
    int n = eynfs_read_file(0, &sb, &entry, buf, sizeof(buf), 0);
    if (n <= 0) {
        printf("[hexdump] Failed to read file\n");
        return;
    }
    printf("[hexdump] %s (%d bytes):\n", filename, n);
    for (int i = 0; i < n; i += 16) {
        printf("%04d: ", i);
        for (int j = 0; j < 16 && i + j < n; ++j) {
            uint8_t val = buf[i + j];
            char hex[3];
            const char* hexchars = "0123456789ABCDEF";
            hex[0] = hexchars[(val >> 4) & 0xF];
            hex[1] = hexchars[val & 0xF];
            hex[2] = 0;
            printf("%s ", hex);
        }
        printf("\n");
    }
} 

 