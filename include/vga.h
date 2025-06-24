#ifndef VGA_H
#define VGA_H

#include "multiboot.h"

void drawRect(int x, int y, int w, int h, int r, int g, int b);
void drawText(int charnum, int r, int g, int b);
void printf(const char* format, ...);
void drawPixel(int x, int y, int r, int g, int b);
void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);
void clearScreen();

#endif