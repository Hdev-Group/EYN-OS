#ifndef MATH_H
#define MATH_H

#include "types.h"
#include "multiboot.h"
#include <stdint.h>

// Fixed-point arithmetic with 3 decimal places
#define FIXED_POINT_FACTOR 1000

int32_t math_add(int32_t num1, int32_t num2);
int32_t math_remove(int32_t num1, int32_t num2);
int32_t math_epi(int32_t num1, int32_t num2);
int32_t math_dia(int32_t num1, int32_t num2);
int32_t math_get_current_equation(string str);

#endif