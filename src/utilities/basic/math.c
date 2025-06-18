#include "../../../include/string.h"
#include "../../../include/vga.h"
#include "../../../include/util.h"
#include "../../../include/multiboot.h"
#include "../../../include/math.h"

int32_t math_add(int32_t num1, int32_t num2) {
	return (num1 + num2);
}

int32_t math_remove(int32_t num1, int32_t num2) {
	return (num1 - num2);
}

int32_t math_epi(int32_t num1, int32_t num2) {
	// Prevent overflow by dividing first
	int32_t temp = num1 / FIXED_POINT_FACTOR;
	return temp * num2;
}

int32_t math_dia(int32_t num1, int32_t num2) {
	if (num2 == 0) return 0; // Prevent division by zero
	// Prevent overflow by dividing first
	return (num1 / num2) * FIXED_POINT_FACTOR;
}

int32_t math_get_current_equation(string str, multiboot_info_t *mbi) {
	if (!str) return 0;
	
	uint8 size = strlength(str);
	if (size == 0) return 0;
	
	int32_t num1 = 0;
	int32_t num2 = 0;
	uint8 operation = 0;
	uint8 i = 0;
	uint8 found_operation = 0;
	uint8 decimal_digits = 0;
	
	// Skip leading spaces
	while (i < size && str[i] == ' ') i++;
	
	// Parse first number
	while (i < size) {
		if (str[i] == '.') {
			if (decimal_digits > 0) return 0; // Multiple decimal points
			decimal_digits = 1;
		}
		else if (str[i] >= '0' && str[i] <= '9') {
			// Check for overflow before multiplying
			if (num1 > INT32_MAX / 10) return 0;
			num1 = num1 * 10 + (str[i] - '0');
			if (decimal_digits > 0) {
				if (decimal_digits > 3) break; // Limit to 3 decimal places
				decimal_digits++;
			}
		}
		else if (str[i] == ' ') {
			break;
		}
		else {
			break;
		}
		i++;
	}
	
	// Apply decimal places to first number
	if (decimal_digits > 0) {
		// Convert to fixed-point by multiplying by appropriate power of 10
		while (decimal_digits < 4) {
			if (num1 > INT32_MAX / 10) return 0;
			num1 *= 10;
			decimal_digits++;
		}
	} else {
		if (num1 > INT32_MAX / FIXED_POINT_FACTOR) return 0;
		num1 *= FIXED_POINT_FACTOR;
	}
	
	// Skip spaces
	while (i < size && str[i] == ' ') i++;
	
	// Get operation
	if (i >= size) return 0;
	
	switch (str[i]) {
		case '+': operation = 0; found_operation = 1; break;
		case '-': operation = 1; found_operation = 1; break;
		case '*': operation = 2; found_operation = 1; break;
		case '/': operation = 3; found_operation = 1; break;
		default: return 0;
	}
	i++;
	
	// Skip spaces
	while (i < size && str[i] == ' ') i++;
	
	// Reset decimal parsing for second number
	decimal_digits = 0;
	
	// Parse second number
	while (i < size) {
		if (str[i] == '.') {
			if (decimal_digits > 0) return 0; // Multiple decimal points
			decimal_digits = 1;
		}
		else if (str[i] >= '0' && str[i] <= '9') {
			// Check for overflow before multiplying
			if (num2 > INT32_MAX / 10) return 0;
			num2 = num2 * 10 + (str[i] - '0');
			if (decimal_digits > 0) {
				if (decimal_digits > 3) break; // Limit to 3 decimal places
				decimal_digits++;
			}
		}
		else if (str[i] == ' ') {
			break;
		}
		else {
			break;
		}
		i++;
	}
	
	// Apply decimal places to second number
	if (decimal_digits > 0) {
		// Convert to fixed-point by multiplying by appropriate power of 10
		while (decimal_digits < 4) {
			if (num2 > INT32_MAX / 10) return 0;
			num2 *= 10;
			decimal_digits++;
		}
	} else {
		if (num2 > INT32_MAX / FIXED_POINT_FACTOR) return 0;
		num2 *= FIXED_POINT_FACTOR;
	}
	
	// Skip trailing spaces
	while (i < size && str[i] == ' ') i++;
	
	// Check if we've processed the entire string
	if (i < size || !found_operation) return 0;
	
	// Perform calculation
	switch (operation) {
		case 0: return math_add(num1, num2);
		case 1: return math_remove(num1, num2);
		case 2: return math_epi(num1, num2);
		case 3: return math_dia(num1, num2);
		default: return 0;
	}
}