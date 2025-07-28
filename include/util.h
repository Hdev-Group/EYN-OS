#ifndef UTIL_H
#define UTIL_H

#include "shell.h"
#include "types.h"

void memory_copy(char *source, char *dest, int nbytes);
void memory_set(uint8 *dest, uint8 val, uint32 len);
string int_to_ascii(int n, char str[]);  
int str_to_int(string ch);
void * my_malloc(int nbytes);  
void my_free(void* ptr);
void init_memory_manager();
void* my_realloc(void* ptr, int new_size);
void* my_calloc(int count, int size);
void print_memory_stats();
string int_to_string(int n);   
uint8 check_string(string str);
void putchar(char c);

extern volatile int g_user_interrupt;

#endif
