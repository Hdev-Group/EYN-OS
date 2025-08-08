#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include "multiboot.h"

void shell_log_enable();
void shell_log_disable();

void print_hex(unsigned int value, int width, int uppercase);
void drawRect(int x, int y, int w, int h, int r, int g, int b);
void drawText(int charnum, int r, int g, int b);
void printf(const char* format, ...);
void drawPixel(int x, int y, int r, int g, int b);
void drawLine(int x1, int y1, int x2, int y2, int r, int g, int b);
void clearScreen();
void start_shell_redirect();
void stop_shell_redirect();
void vga_set_color(int r, int g, int b);

// Markdown rendering functions
void render_markdown(const char* content);
void drawText_bold(int charnum, int r, int g, int b);
void drawText_italic(int charnum, int r, int g, int b);
void drawText_large(int charnum, int r, int g, int b);

#define SHELL_REDIRECT_BUF_SIZE 65536  // 64KB buffer
extern int shell_redirect_active;
extern char shell_redirect_buf[SHELL_REDIRECT_BUF_SIZE];
extern int shell_redirect_pos;

#define LOG_BUF_SIZE 1024
extern char shell_log_buf[LOG_BUF_SIZE];
extern int shell_log_pos;
void shell_log_flush();

int snprintf(char *str, size_t size, const char *format, ...);

#endif