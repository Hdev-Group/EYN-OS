# EYN-OS Math Utilities

The EYN-OS math utilities library provides essential mathematical functions for the operating system, including random number generation, sorting algorithms, and search functions.

## Overview

The math utilities are designed to be efficient and lightweight, suitable for EYN-OS's target hardware (systems with limited memory). All functions are implemented from scratch without external dependencies.

## Random Number Generation

### Linear Congruential Generator (LCG)

EYN-OS uses a Linear Congruential Generator for pseudo-random number generation. This algorithm is simple, fast, and memory-efficient.

#### Algorithm
```c
next = (a * current + c) mod m
```

Where:
- `a = 1103515245` (multiplier)
- `c = 12345` (increment)
- `m = 2^31` (modulus)

#### Functions

##### `rand_seed(uint32_t seed)`
Sets the initial seed for the random number generator.
```c
rand_seed(42);  // Set seed to 42
```

##### `uint32_t rand_next()`
Generates the next random number in the sequence.
```c
uint32_t random_num = rand_next();  // Get random number
```

##### `uint32_t rand_range(uint32_t min, uint32_t max)`
Generates a random number within a specified range.
```c
uint32_t dice_roll = rand_range(1, 6);  // Random number 1-6
```

#### Usage Example
```c
// Initialize random number generator
rand_seed(12345);

// Generate random numbers
for (int i = 0; i < 10; i++) {
    uint32_t num = rand_next();
    printf("Random: %d\n", num);
}

// Generate numbers in range
uint32_t random_choice = rand_range(0, 9);  // 0-9
```

#### Characteristics
- **Period**: 2^31 - 1 (approximately 2 billion numbers)
- **Speed**: Very fast, minimal CPU usage
- **Memory**: Only requires one 32-bit state variable
- **Quality**: Suitable for games and basic applications

## Sorting Algorithms

### Quicksort Implementation

EYN-OS includes a quicksort implementation optimized for string arrays.

#### Functions

##### `void quicksort_strings(char** arr, int low, int high)`
Sorts an array of strings using the quicksort algorithm.
```c
char* strings[] = {"zebra", "apple", "banana", "cat"};
quicksort_strings(strings, 0, 3);
// Result: {"apple", "banana", "cat", "zebra"}
```

##### `int partition_strings(char** arr, int low, int high)`
Partitions the array for quicksort (internal function).
```c
int pivot = partition_strings(arr, low, high);
```

##### `void swap_strings(char** a, char** b)`
Swaps two string pointers (internal function).
```c
swap_strings(&arr[i], &arr[j]);
```

#### Algorithm Details
- **Pivot Selection**: Uses the last element as pivot
- **Partitioning**: Lomuto partition scheme
- **Recursion**: Recursive implementation
- **Performance**: O(n log n) average case, O(n²) worst case

#### Usage Example
```c
// Sort command list for help display
char* commands[] = {"ls", "cd", "help", "exit", "clear"};
int num_commands = 5;

quicksort_strings(commands, 0, num_commands - 1);

// Display sorted commands
for (int i = 0; i < num_commands; i++) {
    printf("%s\n", commands[i]);
}
```

### Bubble Sort

A simple sorting algorithm used for small arrays and educational purposes.

#### Usage
```c
// Simple bubble sort for small arrays
for (int i = 0; i < n-1; i++) {
    for (int j = 0; j < n-i-1; j++) {
        if (strcmp(arr[j], arr[j+1]) > 0) {
            swap_strings(&arr[j], &arr[j+1]);
        }
    }
}
```

## Search Algorithms

### Boyer-Moore String Search

EYN-OS implements the Boyer-Moore algorithm for efficient string searching.

#### Algorithm Overview
The Boyer-Moore algorithm is one of the most efficient string searching algorithms, with average-case performance of O(n/m) where n is the text length and m is the pattern length.

#### Functions

##### `int boyer_moore_search(const char* text, const char* pattern)`
Searches for a pattern within text using Boyer-Moore algorithm.
```c
const char* text = "Hello, World!";
const char* pattern = "World";
int position = boyer_moore_search(text, pattern);
// Returns 7 (position of "World")
```

##### `void build_bad_char_table(const char* pattern, int* bad_char)`
Builds the bad character table for Boyer-Moore (internal function).
```c
int bad_char[256];
build_bad_char_table(pattern, bad_char);
```

##### `void build_good_suffix_table(const char* pattern, int* good_suffix)`
Builds the good suffix table for Boyer-Moore (internal function).
```c
int good_suffix[256];
build_good_suffix_table(pattern, good_suffix);
```

#### Algorithm Steps
1. **Preprocessing**: Build bad character and good suffix tables
2. **Search**: Compare pattern from right to left
3. **Shift**: Use tables to determine optimal shift distance
4. **Match**: Return position when pattern is found

#### Usage Example
```c
// Search for text in file content
const char* file_content = "This is a test file with some content.";
const char* search_term = "test";

int found_pos = boyer_moore_search(file_content, search_term);
if (found_pos != -1) {
    printf("Found '%s' at position %d\n", search_term, found_pos);
} else {
    printf("'%s' not found\n", search_term);
}
```

#### Performance Characteristics
- **Best Case**: O(n/m) - pattern found quickly
- **Average Case**: O(n/m) - very efficient for most cases
- **Worst Case**: O(mn) - rare pathological cases
- **Memory**: O(k) where k is alphabet size

## Integration with EYN-OS

### Command Integration

#### Random Command
```c
void random_cmd(string args) {
    if (!args || strlen(args) == 0) {
        // Generate random number 0-32767
        uint32_t num = rand_next();
        printf("Random number: %d\n", num);
    } else {
        // Parse max value and generate in range
        uint32_t max = parse_number(args);
        uint32_t num = rand_range(0, max);
        printf("Random number (0-%d): %d\n", max, num);
    }
}
```

#### Sort Command
```c
void sort_cmd(string args) {
    if (!args) {
        printf("Usage: sort <filename>\n");
        return;
    }
    
    // Read file lines into array
    char* lines[MAX_LINES];
    int line_count = read_file_lines(args, lines);
    
    // Sort lines
    quicksort_strings(lines, 0, line_count - 1);
    
    // Write sorted lines back to file
    write_sorted_lines(args, lines, line_count);
}
```

#### Search Command
```c
void search_cmd(string args) {
    // Parse search options
    int search_filenames = 1;
    int search_contents = 1;
    
    // Search filesystem recursively
    search_recursive("/", args, search_filenames, search_contents);
}
```

### Memory Management

#### Allocation
All math utilities use EYN-OS's memory management functions:
```c
// Allocate memory for temporary arrays
char** temp_array = my_malloc(sizeof(char*) * array_size);

// Free memory when done
my_free(temp_array);
```

#### Memory Efficiency
- **Minimal Overhead**: Functions designed for low memory usage
- **Stack Usage**: Minimal stack space for recursive functions
- **Temporary Storage**: Efficient use of temporary buffers

## Error Handling

### Input Validation
```c
// Validate range parameters
if (min > max) {
    return min;  // Return min if range is invalid
}

// Validate array bounds
if (low < 0 || high < low || high >= array_size) {
    return;  // Invalid bounds
}
```

### Error Recovery
- **Graceful Degradation**: Functions handle invalid input gracefully
- **Default Values**: Return sensible defaults for edge cases
- **Bounds Checking**: Prevent buffer overflows and invalid access

## Performance Considerations

### Optimization Strategies
- **Algorithm Choice**: Select appropriate algorithm for data size
- **Memory Access**: Minimize memory allocations and copies
- **Loop Optimization**: Use efficient loop structures
- **Function Inlining**: Critical functions may be inlined

### Benchmarking
- **Random Generation**: ~1 million numbers per second
- **String Sorting**: ~1000 strings per second (quicksort)
- **String Search**: ~10MB text per second (Boyer-Moore)

## Future Enhancements

### Planned Features
- **Cryptographic Random**: More secure random number generation
- **Advanced Sorting**: Merge sort, heap sort implementations
- **Pattern Matching**: Regular expression support
- **Mathematical Functions**: Trigonometric, logarithmic functions
- **Optimization**: SIMD instructions for better performance

### Extensibility
- **Plugin System**: Loadable mathematical libraries
- **Custom Algorithms**: User-defined sorting and search functions
- **Benchmarking Tools**: Performance measurement utilities
- **Algorithm Visualization**: Educational tools for algorithm understanding

---