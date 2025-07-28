#ifndef SHELL_COMMAND_INFO_H
#define SHELL_COMMAND_INFO_H

typedef struct {
    const char* name;
    const char* description;
    const char* example;
} shell_command_info_t;

// Macro to register a shell command in the .shellcmds linker section
#define REGISTER_SHELL_COMMAND(var, cmd_name, desc, ex) \
    __attribute__((section(".shellcmds"), used)) \
    const shell_command_info_t shell_cmd_info_##var = { cmd_name, desc, ex }

// Externs for start/stop of the section (set by linker script)
extern const shell_command_info_t __start_shellcmds[];
extern const shell_command_info_t __stop_shellcmds[];

#endif // SHELL_COMMAND_INFO_H 