#include "../../../include/types.h"
#include "../../../include/string.h"
#include "../../../include/system.h"
#include "../../../include/shell.h"
#include "../../../include/util.h"
#include "../../../include/kb.h"
#include "../../../include/math.h"
#include "../../../include/multiboot.h"
#include "../../../include/vga.h"
#include "../../../include/fat32.h"
#include "../../../include/eynfs.h"
#include "../../../include/shell_commands.h"
#include "../../../include/fs_commands.h"
#include "../../../include/fdisk_commands.h"
#include "../../../include/format_command.h"
#include "../../../include/write_editor.h"
#include "../../../include/help_tui.h"
#include "../../../include/run_command.h"
#include "../../../include/assemble.h"
#define LOG_BUF_SIZE 1024

extern int shell_log_active;

void __stack_chk_fail_local() {
    return;
}

uint8_t g_current_drive = 0;

// Add a global variable for the current directory path (for now, always root)
char shell_current_path[128] = "/";

// Handler wrappers for commands needing extra context
void handler_cmd(string arg) {
    printf("%c\nNew recursive shell opened.\n", 0, 255, 0);
    launch_shell(1); // Always launches a new shell at depth 1
}
void handler_exit(string arg) {
    printf("%c\nShutting down...\n", 255, 0, 0);
    Shutdown();
}

// Handler for 'assemble' command
void handler_assemble(string arg) {
    // arg: 'assemble <input.asm> <output.eyn>'
    // Parse arguments
    char input[64] = {0};
    char output[64] = {0};
    int i = 0, j = 0;
    // Skip command name
    while (arg[i] && arg[i] != ' ') i++;
    while (arg[i] == ' ') i++;
    // Read input filename
    while (arg[i] && arg[i] != ' ' && j < 63) input[j++] = arg[i++];
    input[j] = 0;
    while (arg[i] == ' ') i++;
    j = 0;
    // Read output filename
    while (arg[i] && arg[i] != ' ' && j < 63) output[j++] = arg[i++];
    output[j] = 0;
    if (input[0] && output[0]) {
        char* argv[] = {"assemble", input, output};
        assemble_main(3, argv);
    } else {
        printf("Usage: assemble <input.asm> <output.eyn>\n");
    }
}

// Table of command names and handlers
typedef void (*shell_cmd_handler_t)(string arg);
typedef struct {
    const char* name;
    shell_cmd_handler_t handler;
} shell_cmd_entry_t;

static const shell_cmd_entry_t shell_cmds[] = {
    { "cmd", handler_cmd },
    { "clear", (shell_cmd_handler_t)clearScreen },
    { "echo", echo },
    { "help", (shell_cmd_handler_t)help_tui },
    { "spam", (shell_cmd_handler_t)joke_spam },
    { "ver", (shell_cmd_handler_t)ver },
    { "draw", draw_cmd_handler },
    { "calc", calc },
    { "lsata", (shell_cmd_handler_t)lsata },
    { "drives", (shell_cmd_handler_t)drives_cmd },
    { "read", cat },
    { "fdisk", fdisk_cmd_handler },
    { "format", format_cmd_handler },
    { "ls", ls },
    { "write", write_cmd },
    { "del", del },
    { "cd", cd},
    { "makedir", makedir },
    { "deldir", deldir },
    { "drive", drive_cmd },
    { "size", size },
    { "exit", handler_exit },
    { "log", log_cmd },
    { "assemble", handler_assemble },
    { "run", run_command },
    { "random", random_cmd },
    { "history", history_cmd },
    { "sort", sort_cmd },
    { "search", search_cmd },
    { "game", game_cmd },
    { "copy", copy_cmd },
    { "move", move_cmd }
};

void handle_shell_command(const char* cmd, int n) {
    // Parse command name (up to first space)
    char name[32];
    int i = 0;
    while (cmd[i] && cmd[i] != ' ' && i < 31) { name[i] = cmd[i]; i++; }
    name[i] = '\0';

    for (size_t j = 0; j < sizeof(shell_cmds)/sizeof(shell_cmds[0]); ++j) {
        if (strcmp(name, shell_cmds[j].name) == 0) {
            shell_cmds[j].handler(cmd);
            return;
        }
    }
    // check if input is empty or space
    uint8 empty = 1;
    for (uint8 ci = 0; cmd[ci]; ci++) {
        if (cmd[ci] != ' ' && cmd[ci] != '\t' && cmd[ci] != '\n' && cmd[ci] != '\r') {
            empty = 0;
            break;
        }
    }
    if (!empty) {
        printf("%c%s isn't a valid command\n", 255, 0, 0, cmd);
    }
}

void launch_shell(int n) {
    while (1) {
        if (shell_log_active) {
            printf("%c[LOG] ", 0, 255, 0);
        }
        // Print prompt: <drive>:<path>! 
        printf("%c%d:%s", 200, 200, 200, g_current_drive, shell_current_path); // white for drive:path
        printf("%c! ", 255, 255, 0); // yellow for !
        string ch = readStr_with_history(&g_command_history);
        if (shell_log_active) {
            char logline[256];
            int pos = 0;
            // Only log the user input, not the prompt
            for (int i = 0; ch[i] && pos < 254; i++) {
                logline[pos++] = ch[i];
            }
            logline[pos++] = '\n';
            logline[pos] = '\0';
            for (int k = 0; logline[k] && shell_log_pos < LOG_BUF_SIZE - 1; k++) {
                shell_log_buf[shell_log_pos++] = logline[k];
            }
            shell_log_buf[shell_log_pos] = '\0';
            shell_log_flush();
        }
        printf("\n");

        char cmd[200], filename[64];
        int is_redirect = parse_redirection(ch, cmd, filename);

        if (is_redirect) {
            start_shell_redirect();
            handle_shell_command(cmd, n);
            int res = write_output_to_file(shell_redirect_buf, strlen(shell_redirect_buf), filename, g_current_drive);
            if (res == 0)
                printf("%cOutput redirected to '%s' successfully.\n", 0, 255, 0, filename);
            else
                printf("%cFailed to write file '%s' (error code: %d).\n", 255, 0, 0, filename, res);
            stop_shell_redirect();
        } else {
            // Add command to history (only if not empty)
            if (ch && strlen(ch) > 0) {
                add_to_history(&g_command_history, ch);
            }
            handle_shell_command(ch, n);
        }

        if (cmdEql(ch, "exit"))
            break;
    }
}
