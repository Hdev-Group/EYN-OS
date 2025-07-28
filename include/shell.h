#ifndef SHELL_H
#define SHELL_H
#include "types.h"

#define MAX_HISTORY_SIZE 50
#define MAX_COMMAND_LENGTH 200

// Command history structure
typedef struct {
    char commands[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];
    int count;
    int current;
} command_history_t;

// Function declarations
void launch_shell(int n);
void handle_shell_command(const char* cmd, int n);
string readStr_with_history(command_history_t* history);
void add_to_history(command_history_t* history, const char* command);
void clear_history(command_history_t* history);
void show_history(command_history_t* history);

// Global history instance
extern command_history_t g_command_history;

#endif
