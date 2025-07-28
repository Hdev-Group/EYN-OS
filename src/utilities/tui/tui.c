#include "../../../include/tui.h"
#include "../../../include/vga.h"
#include "../../../include/system.h"
#include <string.h>
#include <stdint.h>

// Screen dimensions (to be set by tui_init)
static int tui_screen_width = 80;
static int tui_screen_height = 25;

// Internal cursor position for manual text placement
static int tui_cur_x = 0;
static int tui_cur_y = 0;

// Helper to set the global cursor position for drawText
static void tui_set_cursor(int x, int y) {
    tui_cur_x = x;
    tui_cur_y = y;
    extern int width, height;
    width = x * 8;
    height = y * 8;
}

void tui_init(int screen_width, int screen_height) {
    tui_screen_width = screen_width;
    tui_screen_height = screen_height;
}

void tui_clear() {
    clearScreen();
}

void tui_refresh() {
    // No buffering needed
}

void tui_draw_text(int x, int y, const char* text, tui_style_t style) {
    tui_set_cursor(x, y);
    int r = 0, g = 0, b = 0;
    switch (style.fg_color) {
        case TUI_COLOR_YELLOW: r = 255; g = 255; b = 0; break;
        case TUI_COLOR_RED:    r = 255; g = 0;   b = 0; break;
        case TUI_COLOR_MAGENTA:r = 255; g = 0;   b = 255; break;
        case TUI_COLOR_WHITE:  r = 255; g = 255; b = 255; break;
        case TUI_COLOR_BLACK:  r = 0;   g = 0;   b = 0; break;
        case TUI_COLOR_GRAY:   r = 192; g = 192; b = 192; break;
        default:               r = 255; g = 255; b = 255; break;
    }
    for (size_t i = 0; text[i] != '\0'; ++i) {
        drawText(text[i], r, g, b);
    }
}

void tui_draw_window(const tui_window_t* win) {
    // Draw titlebar
    int title_len = strlen(win->title);
    char titlebar[win->width + 1];
    int pad = (win->width - title_len) / 2;
    for (int i = 0; i < win->width; ++i) titlebar[i] = ' ';
    titlebar[win->width] = '\0';
    for (int i = 0; i < title_len && (pad + i) < win->width; ++i) {
        titlebar[pad + i] = win->title[i];
    }
    tui_draw_text(win->x, win->y, titlebar, win->title_style);
    // Draw separator under titlebar (between borders)
    char sep[win->width + 1];
    sep[0] = '|';
    for (int i = 1; i < win->width - 1; ++i) sep[i] = '-';
    sep[win->width - 1] = '|';
    sep[win->width] = '\0';
    tui_draw_text(win->x, win->y + 1, sep, win->border_style);
    // Draw left/right borders
    int bottom = win->y + win->height - 1;
    for (int i = 2; win->y + i < bottom; ++i) {
        tui_draw_text(win->x, win->y + i, "|", win->border_style);
        tui_draw_text(win->x + win->width - 1, win->y + i, "|", win->border_style);
    }
    // Draw bottom border (between borders)
    char bot[win->width + 1];
    bot[0] = '|';
    for (int i = 1; i < win->width - 1; ++i) bot[i] = '-';
    bot[win->width - 1] = '|';
    bot[win->width] = '\0';
    tui_draw_text(win->x, bottom, bot, win->border_style);
}

void tui_draw_list(const tui_window_t* win, const char** items, int item_count, int selected_index, int scroll_offset, tui_style_t style, tui_style_t selected_style) {
    int max_visible = win->height - 3;
    for (int i = 0; i < max_visible && (i + scroll_offset) < item_count; ++i) {
        int idx = i + scroll_offset;
        if (idx == selected_index) {
            tui_draw_text(win->x + 1, win->y + 2 + i, "!", selected_style);
            tui_draw_text(win->x + 2, win->y + 2 + i, items[idx], style);
        } else {
            tui_draw_text(win->x + 1, win->y + 2 + i, items[idx], style);
        }
    }
}

void tui_draw_text_area(const tui_window_t* win, const char* text, int scroll_offset, tui_style_t style) {
    int max_lines = win->height - 3;
    int y = win->y + 2;
    int x = win->x + 1;
    int line = 0, col = 0;
    for (int i = 0; text[i] != '\0' && line < max_lines + scroll_offset; ++i) {
        if (line >= scroll_offset) {
            if (col == 0) {
                tui_set_cursor(x, y + line - scroll_offset);
            }
            char ch[2] = {text[i], '\0'};
            tui_draw_text(x + col, y + line - scroll_offset, ch, style);
        }
        if (text[i] == '\n' || col >= win->width - 3) {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
}

void tui_draw_status_bar(const tui_window_t* win, const char* text, tui_style_t style) {
    // If a window is provided, draw the status bar just below the window; otherwise, draw at the screen bottom.
    int y = (win == NULL) ? (tui_screen_height - 2) : (win->y + win->height);
    int x = (win == NULL) ? 0 : win->x;
    int width = (win == NULL) ? tui_screen_width : win->width;
    char bar[width + 1];
    int len = strlen(text);
    // Left-align the text
    int i = 0;
    for (; i < len && i < width - 1; ++i) bar[i] = text[i];
    for (; i < width - 1; ++i) bar[i] = ' ';
    bar[width - 1] = '\0';
    tui_draw_text(x, y, bar, style);
}

int tui_read_key() {
    uint8_t ctrl_pressed = 0;
    uint8_t shift_pressed = 0;
    uint8_t caps_lock = 0;
    while (1) {
        if (inportb(0x64) & 0x1) {
            uint8_t scancode = inportb(0x60);
            if (scancode & 0x80) {
                uint8_t realcode = scancode & 0x7F;
                if (realcode == 42 || realcode == 54) shift_pressed = 0;
                if (realcode == 29) ctrl_pressed = 0;
                continue;
            }
            if (scancode == 42 || scancode == 54) { shift_pressed = 1; continue; }
            if (scancode == 29) { ctrl_pressed = 1; continue; }
            if (scancode == 58) { caps_lock = !caps_lock; continue; } // Caps Lock toggle
            // Ctrl+O and Ctrl+X only if ctrl_pressed
            if (ctrl_pressed) {
                if (scancode == 24) return 0x2001; // Ctrl+O (save)
                if (scancode == 45) return 0x2002; // Ctrl+X (exit)
            }
            // Letter logic with Shift and Caps Lock
            int is_letter = 0;
            char base = 0;
            switch (scancode) {
                case 16: base = 'q'; is_letter = 1; break;
                case 17: base = 'w'; is_letter = 1; break;
                case 18: base = 'e'; is_letter = 1; break;
                case 19: base = 'r'; is_letter = 1; break;
                case 20: base = 't'; is_letter = 1; break;
                case 21: base = 'y'; is_letter = 1; break;
                case 22: base = 'u'; is_letter = 1; break;
                case 23: base = 'i'; is_letter = 1; break;
                case 24: base = 'o'; is_letter = 1; break;
                case 25: base = 'p'; is_letter = 1; break;
                case 30: base = 'a'; is_letter = 1; break;
                case 31: base = 's'; is_letter = 1; break;
                case 32: base = 'd'; is_letter = 1; break;
                case 33: base = 'f'; is_letter = 1; break;
                case 34: base = 'g'; is_letter = 1; break;
                case 35: base = 'h'; is_letter = 1; break;
                case 36: base = 'j'; is_letter = 1; break;
                case 37: base = 'k'; is_letter = 1; break;
                case 38: base = 'l'; is_letter = 1; break;
                case 44: base = 'z'; is_letter = 1; break;
                case 45: base = 'x'; is_letter = 1; break;
                case 46: base = 'c'; is_letter = 1; break;
                case 47: base = 'v'; is_letter = 1; break;
                case 48: base = 'b'; is_letter = 1; break;
                case 49: base = 'n'; is_letter = 1; break;
                case 50: base = 'm'; is_letter = 1; break;
            }
            if (is_letter) {
                int upper = (caps_lock && !shift_pressed) || (!caps_lock && shift_pressed);
                if (upper) return base - 32; // Uppercase
                else return base; // Lowercase
            }
            switch (scancode) {
                case 72: return 0x1001; // Up arrow
                case 80: return 0x1002; // Down arrow
                case 75: return 0x1003; // Left arrow
                case 77: return 0x1004; // Right arrow
                case 14: return '\b';   // Backspace
                case 28: return '\n';   // Enter
                case 1:  return 27;     // Escape
                case 83: return 0x1005; // Delete
                case 71: return 0x1006; // Home
                case 73: return 0x1008; // Page Up
                case 79: return 0x1007; // End
                case 81: return 0x1009; // Page Down
                case 2: return shift_pressed ? '!' : '1';
                case 3: return shift_pressed ? '@' : '2';
                case 4: return shift_pressed ? '#' : '3';
                case 5: return shift_pressed ? '$' : '4';
                case 6: return shift_pressed ? '%' : '5';
                case 7: return shift_pressed ? '^' : '6';
                case 8: return shift_pressed ? '&' : '7';
                case 9: return shift_pressed ? '*' : '8';
                case 10: return shift_pressed ? '(' : '9';
                case 11: return shift_pressed ? ')' : '0';
                case 12: return shift_pressed ? '_' : '-';
                case 13: return shift_pressed ? '+' : '=';
                case 26: return shift_pressed ? '{' : '[';
                case 27: return shift_pressed ? '}' : ']';
                case 39: return shift_pressed ? ':' : ';';
                case 40: return shift_pressed ? '"' : '\'';
                case 41: return shift_pressed ? '~' : '`';
                case 43: return shift_pressed ? '|' : '\\';
                case 51: return shift_pressed ? '<' : ',';
                case 52: return shift_pressed ? '>' : '.';
                case 53: return shift_pressed ? '?' : '/';
                case 57: return ' '; // Space
            }
        }
    }
} 