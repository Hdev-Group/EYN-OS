#ifndef STRING_H
#define STRING_H

#include "types.h"
#include <stddef.h>
uint16 strlength(string ch);

char* strstr(const char* haystack, const char* needle); 
uint8 strEql(string ch1,string ch2);
uint8 cmdEql(string ch1, string ch2);
uint8 cmdLength(string ch);
uint8 whereSpace1(string ch1);
uint8 searchArgMain(string ch1, string ch2);
uint8 whatIsArgMain(string ch1);
uint8 isStringEmpty(string ch1);

// Standard C-like memory/string functions for kernel use
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);
char *strtok_r(char *str, const char *delim, char **saveptr);

// Prototypes for custom kernel string/memory functions
size_t strspn(const char *s, const char *accept);
char *strpbrk(const char *s, const char *accept);
size_t strlen(const char *s);

uint32 str_to_uint(const char* s);
int atoi(const char* s);
int parse_redirection(const char* input, char* cmd, char* filename);
void echo_to_buf(string ch, char* outbuf, int outbufsize);
void calc_to_buf(string str, char* outbuf, int outbufsize);

char *strcpy(char *dest, const char *src);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char* strncat(char* dest, const char* src, unsigned int n);

#endif
