#ifndef SHELL_COMMANDS_H
#define SHELL_COMMANDS_H
#include "types.h"

void echo(string ch);
void help();
void ver();
void joke_spam();
void draw_cmd_handler(string ch);
void calc(string ch);
void lsata();
void drives_cmd(string ch);
void drive_cmd(string ch);
void memory_cmd(string ch);
void size(string ch);
void log_cmd(string ch);
void hexdump_cmd(string ch);
void random_cmd(string ch);
void history_cmd(string ch);
void sort_cmd(string ch);
void search_cmd(string ch);
void game_cmd(string ch);

#endif 