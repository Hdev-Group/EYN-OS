#include "../../../include/string.h"
#include "../../../include/vga.h"
#include "../../../include/util.h"
#include <stddef.h>

uint16 strlength(string ch)
{
        uint16 i = 0;           //Changed counter to 0
        while(ch[i++]);
        return i-1;               //Changed counter to i instead of i--
}

uint8 cmdLength(string ch) {
        uint8 i = 0;
        uint8 size = 0;
        uint8 realSize = strlength(ch);
        uint8 hasSpace = 0;

        // If the string is empty, return 0
        if (realSize == 0) return 0;

        // If there's no space, return the full length
        for (i = 0; i < realSize; i++) {
            if (ch[i] == ' ') {
                hasSpace = 1;
                break;
            }
            size++;
        }
        
        return size;
}

uint8 isStringEmpty(string ch1) {
        if (ch1[0] == '1') {return 0;}
        else if (ch1[0] == '2') {return 0;}
        else if (ch1[0] == '3') {return 0;}
        else if (ch1[0] == '4') {return 0;}
        else if (ch1[0] == '5') {return 0;}
        else if (ch1[0] == '6') {return 0;}
        else if (ch1[0] == '7') {return 0;}
        else if (ch1[0] == '8') {return 0;}
        else if (ch1[0] == '9') {return 0;}
        else if (ch1[0] == '0') {return 0;}
        else if (ch1[0] == '-') {return 0;}
        else if (ch1[0] == '=') {return 0;}
        else if (ch1[0] == 'q') {return 0;}
        else if (ch1[0] == 'w') {return 0;}
        else if (ch1[0] == 'e') {return 0;}
        else if (ch1[0] == 'r') {return 0;}
        else if (ch1[0] == 't') {return 0;}
        else if (ch1[0] == 'y') {return 0;}
        else if (ch1[0] == 'u') {return 0;}
        else if (ch1[0] == 'i') {return 0;}
        else if (ch1[0] == 'o') {return 0;}
        else if (ch1[0] == 'p') {return 0;}
        else if (ch1[0] == '[') {return 0;}
        else if (ch1[0] == ']') {return 0;}
        else if (ch1[0] == '\\') {return 0;}
        else if (ch1[0] == 'a') {return 0;}
        else if (ch1[0] == 's') {return 0;}
        else if (ch1[0] == 'd') {return 0;}
        else if (ch1[0] == 'f') {return 0;}
        else if (ch1[0] == 'g') {return 0;}
        else if (ch1[0] == 'h') {return 0;}
        else if (ch1[0] == 'j') {return 0;}
        else if (ch1[0] == 'k') {return 0;}
        else if (ch1[0] == 'l') {return 0;}
        else if (ch1[0] == ';') {return 0;}
        else if (ch1[0] == '\'') {return 0;}
        else if (ch1[0] == 'z') {return 0;}
        else if (ch1[0] == 'x') {return 0;}
        else if (ch1[0] == 'c') {return 0;}
        else if (ch1[0] == 'v') {return 0;}
        else if (ch1[0] == 'b') {return 0;}
        else if (ch1[0] == 'n') {return 0;}
        else if (ch1[0] == 'm') {return 0;}
        else if (ch1[0] == ',') {return 0;}
        else if (ch1[0] == '.') {return 0;}
        else if (ch1[0] == '/') {return 0;}
        else {
                return 1;
        }
}

uint8 strEql(string ch1,string ch2)
{
        uint8 result = 1;
        uint8 size = strlength(ch1);
        if(size != strlength(ch2)) result = 0;
        else
        {
                uint8 i = 0;
                for(i; i < size; i++)  // Changed <= to < to avoid reading past null terminator
                {
                        if(ch1[i] != ch2[i]) result = 0;
                }
        }
        return result;
}

uint8 cmdEql(string ch1, string ch2) {
        uint8 res = 1;
        uint8 size = cmdLength(ch1);

        if (size != cmdLength(ch2)) res = 0;
        else {
                uint8 i = 0;
                for (i; i < size; i++) {  // Changed <= to < to avoid reading past null terminator
                        if (ch1[i] != ch2[i]) {res = 0;}
                }
        }

        return res;
}

// Minimal kernel implementations of standard C functions
void *memcpy(void *dest, const void *src, size_t n) {
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    for (size_t i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0')
            return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token;
    if (str == NULL) str = *saveptr;
    // Skip leading delimiters
    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }
    token = str;
    str = strpbrk(token, delim);
    if (str) {
        *str = '\0';
        *saveptr = str + 1;
    } else {
        *saveptr = token + strlen(token);
    }
    return token;
}

// Helper functions for strtok_r
size_t strspn(const char *s, const char *accept) {
    const char *p;
    size_t count = 0;
    while (*s) {
        for (p = accept; *p; p++) {
            if (*s == *p) break;
        }
        if (!*p) break;
        s++; count++;
    }
    return count;
}

char *strpbrk(const char *s, const char *accept) {
    while (*s) {
        for (const char *a = accept; *a; a++) {
            if (*s == *a) return (char *)s;
        }
        s++;
    }
    return NULL;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

// Converts a string to uint32 (decimal only)
uint32 str_to_uint(const char* s) {
    uint32 n = 0;
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

// Parses redirection: e.g. "echo hi > file.txt" -> cmd="echo hi", filename="file.txt"
int parse_redirection(const char* input, char* cmd, char* filename) {
    int i = 0, j = 0, k = 0;
    int found = 0;
    while (input[i]) {
        if (input[i] == '>') {
            found = 1;
            break;
        }
        cmd[j++] = input[i++];
    }
    cmd[j] = '\0';
    if (!found) return 0;
    i++; // skip '>'
    while (input[i] == ' ') i++;
    while (input[i] && input[i] != ' ' && k < 63) filename[k++] = input[i++];
    filename[k] = '\0';
    return 1;
}

// Copies the argument after 'echo' to outbuf
void echo_to_buf(string ch, char* outbuf, int outbufsize) {
    int i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] == ' ') i++;
    int j = 0;
    while (ch[i] && j < outbufsize - 1) outbuf[j++] = ch[i++];
    outbuf[j] = '\0';
}

// Evaluates a simple integer expression after 'calc' and writes result to outbuf
void calc_to_buf(string str, char* outbuf, int outbufsize) {
    int i = 0;
    while (str[i] && str[i] != ' ') i++;
    while (str[i] == ' ') i++;
    // Only support: calc <int> <op> <int>
    int a = 0, b = 0;
    char op = 0;
    int sign = 1;
    if (str[i] == '-') { sign = -1; i++; }
    while (str[i] >= '0' && str[i] <= '9') { a = a * 10 + (str[i++] - '0'); }
    a *= sign;
    while (str[i] == ' ') i++;
    op = str[i++];
    while (str[i] == ' ') i++;
    sign = 1;
    if (str[i] == '-') { sign = -1; i++; }
    while (str[i] >= '0' && str[i] <= '9') { b = b * 10 + (str[i++] - '0'); }
    b *= sign;
    int res = 0;
    switch (op) {
        case '+': res = a + b; break;
        case '-': res = a - b; break;
        case '*': res = a * b; break;
        case '/': res = (b != 0) ? a / b : 0; break;
        default: {
            const char* msg = "Invalid op";
            int j = 0;
            while (msg[j] && j < outbufsize - 1) { outbuf[j] = msg[j]; j++; }
            outbuf[j] = '\0';
            return;
        }
    }
    string tmp = int_to_string(res);
    int j = 0;
    while (tmp[j] && j < outbufsize - 1) { outbuf[j] = tmp[j]; j++; }
    outbuf[j] = '\0';
}