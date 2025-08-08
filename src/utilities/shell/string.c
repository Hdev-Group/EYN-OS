#include <string.h>
#include <vga.h>
#include <util.h>
#include <stddef.h>

// Input validation and buffer overflow protection
static int input_validation_errors = 0;

// Validate string bounds and content
int validate_string(const char* str, int max_length) {
    if (!str) return 0;
    
    // Check for null bytes and excessive length
    for (int i = 0; i < max_length; i++) {
        if (str[i] == '\0') break;
        if (str[i] < 32 || str[i] > 126) {
            // Invalid character (outside printable ASCII)
            return 0;
        }
    }
    
    return 1;
}

// Safe string copy with bounds checking
char* safe_strcpy(char* dest, const char* src, int dest_size) {
    if (!dest || !src || dest_size <= 0) {
        input_validation_errors++;
        return NULL;
    }
    
    // Validate source string
    if (!validate_string(src, dest_size)) {
        input_validation_errors++;
        return NULL;
    }
    
    int i = 0;
    while (i < dest_size - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0'; // Always null-terminate
    
    return dest;
}

// Safe string concatenation with bounds checking
char* safe_strcat(char* dest, const char* src, int dest_size) {
    if (!dest || !src || dest_size <= 0) {
        input_validation_errors++;
        return NULL;
    }
    
    // Find end of destination string
    int dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0') {
        dest_len++;
    }
    
    // Validate source string
    if (!validate_string(src, dest_size - dest_len)) {
        input_validation_errors++;
        return NULL;
    }
    
    // Concatenate safely
    int i = 0;
    while (dest_len + i < dest_size - 1 && src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0'; // Always null-terminate
    
    return dest;
}

// Validate file path for traversal attacks
int validate_file_path(const char* path) {
    if (!path) return 0;
    
    // Check for path traversal attempts
    if (strstr(path, "..") || strstr(path, "//") || strstr(path, "\\")) {
        return 0; // Potential path traversal
    }
    
    // Check for absolute paths (should be relative)
    if (path[0] == '/') {
        return 0; // Absolute path not allowed
    }
    
    // Validate string content
    return validate_string(path, 256);
}

// Sanitize input string
char* sanitize_input(char* input, int max_length) {
    if (!input || max_length <= 0) return NULL;
    
    // Remove control characters and normalize
    int j = 0;
    for (int i = 0; i < max_length && input[i] != '\0'; i++) {
        if (input[i] >= 32 && input[i] <= 126) {
            input[j++] = input[i];
        }
    }
    input[j] = '\0';
    
    return input;
}

// Get input validation error count
int get_input_validation_errors() {
    return input_validation_errors;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && *h == *n) h++, n++;
        if (!*n) return (char*)haystack;
    }
    return 0;
} 

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

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) return (char *)s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return NULL;
}

char *strrchr(const char *s, int c) {
    const char *last = NULL;
    while (*s) {
        if (*s == c) last = s;
        s++;
    }
    if (c == '\0') return (char *)s;
    return (char *)last;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
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

// Converts a string to integer (decimal only)
int atoi(const char* s) {
    int n = 0;
    int sign = 1;
    
    // Handle sign
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n * sign;
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

// Simple strcpy implementation
char *strcpy(char *dest, const char *src) {
    if (!dest || !src) return dest;
    
    char *d = dest;
    while (*src && (d - dest) < 255) {  // Add reasonable bounds check
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

char* strncat(char* dest, const char* src, unsigned int n) {
    char* d = dest;
    while (*d) d++;
    while (n-- && *src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}