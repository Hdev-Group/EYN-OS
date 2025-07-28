#include "../../include/types.h"
#include "../../include/util.h"
#include "../../include/string.h"
#include "../../include/vga.h"
#include <stdint.h>

volatile int g_user_interrupt = 0;

uint32_t __stack_chk_fail(){
    return 0;
}

void memory_copy(char *source, char *dest, int nbytes) {
    int i;
    for (i = 0; i < nbytes; i++) {
        *(dest + i) = *(source + i);             //    dest[i] = source[i]
    }
}

void memory_set(uint8 *dest, uint8 val, uint32 len) {
    uint8 *temp = (uint8 *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

/**
 * K&R implementation
 */
string int_to_ascii(int n, char str[]) {          
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        str[i++] = n % 10 + '0';         
    } while ((n /= 10) > 0);

    if (sign < 0) str[i++] = '-';
    str[i] = '\0';

    /* TODO: implement "reverse" */
    return str;
}
string int_to_string(int n)
{
	string ch = my_malloc(50);
	int_to_ascii(n,ch);
	int len = strlength(ch);
	int i = 0;
	int j = len - 1;
	while(i<(len/2 + len%2))
	{
		char tmp = ch[i];
		ch[i] = ch[j];
		ch[j] = tmp;
		i++;
		j--;
	}
	return ch;
}

int str_to_int(string ch)
{
	int n = 0;
	int p = 1;
	int strlen = strlength(ch);
	int i;
	for (i = strlen-1;i>=0;i--)
	{
		n += ((int)(ch[i] - '0')) * p;
		p *= 10;
	}
	return n;
}

string char_to_string(char ch) {
	string str;
	str[0] = ch;

	return str;
}

uint8 check_string(string str) {
	uint8 size = strlength(str);
	uint8 res = 0;
	for (uint8 i = 0; i <= size; i++) {
		if (
			str[i] == '1' ||
			str[i] == '2' ||
			str[i] == '3' ||
			str[i] == '4' ||
			str[i] == '5' ||
			str[i] == '6' ||
			str[i] == '7' ||
			str[i] == '8' ||
			str[i] == '9' ||
			str[i] == '-' ||
			str[i] == '=' ||
			str[i] == '0' ||
			str[i] == '!' ||
			str[i] == '@' ||
			str[i] == '#' ||
			str[i] == '$' ||
			str[i] == '%' ||
			str[i] == '^' ||
			str[i] == '&' ||
			str[i] == '*' ||
			str[i] == '(' ||
			str[i] == ')' ||
			str[i] == '_' ||
			str[i] == '+' ||
			str[i] == 'q' ||
			str[i] == 'w' ||
			str[i] == 'e' ||
			str[i] == 'r' ||
			str[i] == 't' ||
			str[i] == 'y' ||
			str[i] == 'u' ||
			str[i] == 'i' ||
			str[i] == 'o' ||
			str[i] == 'p' ||
			str[i] == '[' ||
			str[i] == ']' ||
			str[i] == '\\' ||
			str[i] == 'Q' ||
			str[i] == 'W' ||
			str[i] == 'E' ||
			str[i] == 'R' ||
			str[i] == 'T' ||
			str[i] == 'Y' ||
			str[i] == 'U' ||
			str[i] == 'I' ||
			str[i] == 'O' ||
			str[i] == 'P' ||
			str[i] == '{' ||
			str[i] == '}' ||
			str[i] == '|' ||
			str[i] == 'a' ||
			str[i] == 's' ||
			str[i] == 'd' ||
			str[i] == 'f' ||
			str[i] == 'g' ||
			str[i] == 'h' ||
			str[i] == 'j' ||
			str[i] == 'k' ||
			str[i] == 'l' ||
			str[i] == ';' ||
			str[i] == '\'' ||
			str[i] == 'A' ||
			str[i] == 'S' ||
			str[i] == 'D' ||
			str[i] == 'F' ||
			str[i] == 'G' ||
			str[i] == 'H' ||
			str[i] == 'J' ||
			str[i] == 'K' ||
			str[i] == 'L' ||
			str[i] == ':' ||
			str[i] == '"' ||
			str[i] == 'z' ||
			str[i] == 'x' ||
			str[i] == 'c' ||
			str[i] == 'v' ||
			str[i] == 'b' ||
			str[i] == 'n' ||
			str[i] == 'm' ||
			str[i] == ',' ||
			str[i] == '.' ||
			str[i] == '/' ||
			str[i] == 'Z' ||
			str[i] == 'X' ||
			str[i] == 'C' ||
			str[i] == 'V' ||
			str[i] == 'B' ||
			str[i] == 'N' ||
			str[i] == 'M' ||
			str[i] == '<' ||
			str[i] == '>' ||
			str[i] == '?'
		) {
			res = 1;
		}
	}

	return res;
}

uint8 check_string_numbers(string str) {
	uint8 size = strlength(str);
	uint8 res = 0;
	for (uint8 i = 0; i <= size; i++) {
		if (
			str[i] == '1' ||
			str[i] == '2' ||
			str[i] == '3' ||
			str[i] == '4' ||
			str[i] == '5' ||
			str[i] == '6' ||
			str[i] == '7' ||
			str[i] == '8' ||
			str[i] == '9' ||
			str[i] == '0'
		) {
			res = 1;
		}
	}
	return res;
}

// --- Robust Heap Manager Rewrite ---
#define HEAP_START 0x1000000  // 16MB - safe area for heap
#define HEAP_SIZE 0x1000000   // 16MB heap size
#define BLOCK_HEADER_SIZE 16
#define MIN_BLOCK_SIZE 32
#define NO_BLOCK 0xFFFFFFFF

typedef struct {
    uint32_t size;        // Size of the block (including header)
    uint32_t used;        // 1 if used, 0 if free
    uint32_t next;        // Offset to next block (NO_BLOCK if last)
    uint32_t prev;        // Offset to previous block (NO_BLOCK if first)
} block_header_t;

static uint8_t* heap_start = (uint8_t*)HEAP_START;
static uint32_t first_block = 0;
static int memory_initialized = 0;

void init_memory_manager() {
    if (memory_initialized) return;
    memory_set(heap_start, 0, HEAP_SIZE);
    block_header_t* first = (block_header_t*)heap_start;
    first->size = HEAP_SIZE;
    first->used = 0;
    first->next = NO_BLOCK;
    first->prev = NO_BLOCK;
    first_block = 0;
    memory_initialized = 1;
}

static uint32_t find_free_block(uint32_t size) {
    uint32_t current = first_block;
    while (current != NO_BLOCK) {
        block_header_t* block = (block_header_t*)(heap_start + current);
        if (!block->used && block->size >= size) {
            return current;
        }
        current = block->next;
    }
    return NO_BLOCK;
}

static void split_block(uint32_t block_offset, uint32_t needed_size) {
    block_header_t* block = (block_header_t*)(heap_start + block_offset);
    if (block->size < needed_size + BLOCK_HEADER_SIZE + MIN_BLOCK_SIZE) {
        return; // Don't split if the remainder would be too small
    }
    uint32_t new_block_offset = block_offset + needed_size;
    block_header_t* new_block = (block_header_t*)(heap_start + new_block_offset);
    new_block->size = block->size - needed_size;
    new_block->used = 0;
    new_block->next = block->next;
    new_block->prev = block_offset;
    if (block->next != NO_BLOCK) {
        block_header_t* next_block = (block_header_t*)(heap_start + block->next);
        next_block->prev = new_block_offset;
    }
    block->size = needed_size;
    block->next = new_block_offset;
}

static void merge_free_blocks() {
    uint32_t current = first_block;
    while (current != NO_BLOCK) {
        block_header_t* block = (block_header_t*)(heap_start + current);
        while (block->next != NO_BLOCK) {
            block_header_t* next_block = (block_header_t*)(heap_start + block->next);
            if (!block->used && !next_block->used) {
                block->size += next_block->size;
                block->next = next_block->next;
                if (next_block->next != NO_BLOCK) {
                    block_header_t* next_next = (block_header_t*)(heap_start + next_block->next);
                    next_next->prev = current;
                }
            } else {
                break;
            }
        }
        current = block->next;
    }
}

void* my_malloc(int nbytes) {
    if (!memory_initialized) {
        init_memory_manager();
    }
    if (nbytes <= 0) return NULL;
    uint32_t total_size = ((nbytes + BLOCK_HEADER_SIZE + 7) / 8) * 8;
    uint32_t block_offset = find_free_block(total_size);
    if (block_offset == NO_BLOCK) {
        printf("%cmy_malloc: Out of memory\n", 255, 0, 0);
        return NULL;
    }
    block_header_t* block = (block_header_t*)(heap_start + block_offset);
    block->used = 1;
    split_block(block_offset, total_size);
    // Return pointer to the data area (after header)
    return (void*)(heap_start + block_offset + BLOCK_HEADER_SIZE);
}

void my_free(void* ptr) {
    if (!ptr || !memory_initialized) return;
    uint8_t* data_ptr = (uint8_t*)ptr;
    uint32_t block_offset = data_ptr - heap_start - BLOCK_HEADER_SIZE;
    if (block_offset >= HEAP_SIZE) return; // Invalid pointer
    block_header_t* block = (block_header_t*)(heap_start + block_offset);
    if (!block->used) return; // Already freed
    block->used = 0;
    merge_free_blocks();
}

void* my_realloc(void* ptr, int new_size) {
    if (!ptr) return my_malloc(new_size);
    if (new_size <= 0) {
        my_free(ptr);
        return NULL;
    }
    uint8_t* data_ptr = (uint8_t*)ptr;
    uint32_t block_offset = data_ptr - heap_start - BLOCK_HEADER_SIZE;
    if (block_offset >= HEAP_SIZE) return NULL; // Invalid pointer
    block_header_t* block = (block_header_t*)(heap_start + block_offset);
    uint32_t current_size = block->size - BLOCK_HEADER_SIZE;
    if (new_size <= current_size) {
        return ptr; // No need to reallocate
    }
    void* new_ptr = my_malloc(new_size);
    if (!new_ptr) return NULL;
    memory_copy((char*)ptr, (char*)new_ptr, current_size);
    my_free(ptr);
    return new_ptr;
}

void* my_calloc(int count, int size) {
    int total_size = count * size;
    void* ptr = my_malloc(total_size);
    if (ptr) {
        memory_set((uint8*)ptr, 0, total_size);
    }
    return ptr;
}

void print_memory_stats() {
    if (!memory_initialized) {
        printf("%cMemory manager not initialized\n", 255, 0, 0);
        return;
    }
    uint32_t total_free = 0;
    uint32_t total_used = 0;
    uint32_t block_count = 0;
    uint32_t current = first_block;
    while (current != NO_BLOCK) {
        block_header_t* block = (block_header_t*)(heap_start + current);
        if (block->used) {
            total_used += block->size;
        } else {
            total_free += block->size;
        }
        block_count++;
        current = block->next;
    }
    printf("%cMemory Stats:\n", 255, 255, 255);
    printf("%c  Total Heap: %d bytes\n", 255, 255, 255, HEAP_SIZE);
    printf("%c  Used: %d bytes\n", 255, 255, 255, total_used);
    printf("%c  Free: %d bytes\n", 255, 255, 255, total_free);
    printf("%c  Blocks: %d\n", 255, 255, 255, block_count);
}

void putchar(char c) {
    drawText(c, 255, 255, 255);
}