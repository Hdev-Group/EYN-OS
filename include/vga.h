#ifndef VGA_H
#define VGA_H

#include "multiboot.h"

void drawRect(multiboot_info_t *mbi, int x, int y, int w, int h, int r, int g, int b);
void drawText(multiboot_info_t *mbi, int charnum, int r, int g, int b);
void printf(multiboot_info_t *mbi, const char* format, ...);
void drawPixel(multiboot_info_t *mbi, int x, int y, int r, int g, int b);
void drawLine(multiboot_info_t *mbi, int x1, int y1, int x2, int y2, int r, int g, int b);
void clearScreen(multiboot_info_t *mbi);

#endif