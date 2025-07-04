#ifndef UTIL_H
#define UTIL_H

#include "shell.h"
#include "types.h"

void memory_copy(char *source, char *dest, int nbytes);
void memory_set(uint8 *dest, uint8 val, uint32 len);
string int_to_ascii(int n, char str[]);  
int str_to_int(string ch);
void * my_malloc(int nbytes);  
string int_to_string(int n);   
uint8 check_string(string str);
void putchar(char c);

extern volatile int g_user_interrupt;

#endif
