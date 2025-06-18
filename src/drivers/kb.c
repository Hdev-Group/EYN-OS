#include "../../include/kb.h"
#include "../../include/system.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"
#include <stdlib.h>

string readStr(multiboot_info_t *mbi) {
    char buff;
    string buffstr = (string) malloc(200);
    uint8 i = 0;
    uint8 reading = 1;
    uint8 readTmp = 1;
    uint8 shift_pressed = 0;  // Track shift key state
    uint8 caps_lock = 0;      // Track caps lock state
    uint8 ctrl_pressed = 0;   // Track control key state
    
    while(reading)
    {
        if(inportb(0x64) & 0x1)
        {
            uint8 scancode = inportb(0x60);
            
            // Handle modifier keys
            if(scancode == 42 || scancode == 54) {  // Left or Right shift press
                shift_pressed = 1;
                continue;
            }
            if(scancode == 170 || scancode == 182) {  // Left or Right shift release
                shift_pressed = 0;
                continue;
            }
            if(scancode == 58) {  // Caps Lock
                caps_lock = !caps_lock;  // Toggle caps lock state
                continue;
            }
            if(scancode == 29) {  // Left Control press
                ctrl_pressed = 1;
                continue;
            }
            if(scancode == 157) {  // Left Control release
                ctrl_pressed = 0;
                continue;
            }
            
            // Skip key release events
            if(scancode & 0x80) {
                continue;
            }
            
            // Check for Ctrl+C
            if(ctrl_pressed && scancode == 46) {  // 'c' key
                buffstr[0] = '\0';  // Clear the buffer
                reading = 0;        // Exit the reading loop
                printf(mbi, "%c^C\n", 255, 255, 255);  // Print ^C in white
                continue;
            }
            
            // Determine if we should use uppercase (shift XOR caps lock)
            uint8 use_uppercase = shift_pressed ^ caps_lock;
            
            switch(scancode)
            {
                case 1:  // Escape
                        drawText(mbi, (char)27, 255, 255, 255);
                buffstr[i] = (char)27;
                i++;
                        break;
                case 2:
                        if(shift_pressed) {
                            drawText(mbi, '!', 255, 255, 255);
                            buffstr[i] = '!';
                        } else {
                        drawText(mbi, '1', 255, 255, 255);
                        buffstr[i] = '1';
                        }
                        i++;
                        break;
                case 3:
                        if(shift_pressed) {
                            drawText(mbi, '@', 255, 255, 255);
                            buffstr[i] = '@';
                        } else {
                        drawText(mbi, '2', 255, 255, 255);
                        buffstr[i] = '2';
                        }
                        i++;
                        break;
                case 4:
                        if(shift_pressed) {
                            drawText(mbi, '#', 255, 255, 255);
                            buffstr[i] = '#';
                        } else {
                        drawText(mbi, '3', 255, 255, 255);
                        buffstr[i] = '3';
                        }
                        i++;
                        break;
                case 5:
                        if(shift_pressed) {
                            drawText(mbi, '$', 255, 255, 255);
                            buffstr[i] = '$';
                        } else {
                        drawText(mbi, '4', 255, 255, 255);
                        buffstr[i] = '4';
                        }
                        i++;
                        break;
                case 6:
                        if(shift_pressed) {
                            drawText(mbi, '%', 255, 255, 255);
                            buffstr[i] = '%';
                        } else {
                        drawText(mbi, '5', 255, 255, 255);
                        buffstr[i] = '5';
                        }
                        i++;
                        break;
                case 7:
                        if(shift_pressed) {
                            drawText(mbi, '^', 255, 255, 255);
                            buffstr[i] = '^';
                        } else {
                        drawText(mbi, '6', 255, 255, 255);
                        buffstr[i] = '6';
                        }
                        i++;
                        break;
                case 8:
                        if(shift_pressed) {
                            drawText(mbi, '&', 255, 255, 255);
                            buffstr[i] = '&';
                        } else {
                        drawText(mbi, '7', 255, 255, 255);
                        buffstr[i] = '7';
                        }
                        i++;
                        break;
                case 9:
                        if(shift_pressed) {
                            drawText(mbi, '*', 255, 255, 255);
                            buffstr[i] = '*';
                        } else {
                        drawText(mbi, '8', 255, 255, 255);
                        buffstr[i] = '8';
                        }
                        i++;
                        break;
                case 10:
                        if(shift_pressed) {
                            drawText(mbi, '(', 255, 255, 255);
                            buffstr[i] = '(';
                        } else {
                        drawText(mbi, '9', 255, 255, 255);
                        buffstr[i] = '9';
                        }
                        i++;
                        break;
                case 11:
                        if(shift_pressed) {
                            drawText(mbi, ')', 255, 255, 255);
                            buffstr[i] = ')';
                        } else {
                        drawText(mbi, '0', 255, 255, 255);
                        buffstr[i] = '0';
                        }
                        i++;
                        break;
                case 12:
                        if(shift_pressed) {
                            drawText(mbi, '_', 255, 255, 255);
                            buffstr[i] = '_';
                        } else {
                        drawText(mbi, '-', 255, 255, 255);
                        buffstr[i] = '-';
                        }
                        i++;
                        break;
                case 13:
                        if(shift_pressed) {
                            drawText(mbi, '+', 255, 255, 255);
                            buffstr[i] = '+';
                        } else {
                        drawText(mbi, '=', 255, 255, 255);
                        buffstr[i] = '=';
                        }
                        i++;
                        break;
                case 14:  // Backspace
                        if (i == 0) {
                                break;
                        } else {
                                drawText(mbi, '\b', 255, 255, 255);
                                i--;
                                buffstr[i+1] = 0;
                                buffstr[i] = 0;
                                break;
                        }
                case 15:  // Tab
                        drawText(mbi, '\t', 255, 255, 255);
                        buffstr[i] = '\t';
                        i++;
                        break;
                case 16:
                        if(use_uppercase) {
                            drawText(mbi, 'Q', 255, 255, 255);
                            buffstr[i] = 'Q';
                        } else {
                        drawText(mbi, 'q', 255, 255, 255);
                        buffstr[i] = 'q';
                        }
                        i++;
                        break;
                case 17:
                        if(use_uppercase) {
                            drawText(mbi, 'W', 255, 255, 255);
                            buffstr[i] = 'W';
                        } else {
                        drawText(mbi, 'w', 255, 255, 255);
                        buffstr[i] = 'w';
                        }
                        i++;
                        break;
                case 18:
                        if(use_uppercase) {
                            drawText(mbi, 'E', 255, 255, 255);
                            buffstr[i] = 'E';
                        } else {
                        drawText(mbi, 'e', 255, 255, 255);
                        buffstr[i] = 'e';
                        }
                        i++;
                        break;
                case 19:
                        if(use_uppercase) {
                            drawText(mbi, 'R', 255, 255, 255);
                            buffstr[i] = 'R';
                        } else {
                        drawText(mbi, 'r', 255, 255, 255);
                        buffstr[i] = 'r';
                        }
                        i++;
                        break;
                case 20:
                        if(use_uppercase) {
                            drawText(mbi, 'T', 255, 255, 255);
                            buffstr[i] = 'T';
                        } else {
                        drawText(mbi, 't', 255, 255, 255);
                        buffstr[i] = 't';
                        }
                        i++;
                        break;
                case 21:
                        if(use_uppercase) {
                            drawText(mbi, 'Y', 255, 255, 255);
                            buffstr[i] = 'Y';
                        } else {
                        drawText(mbi, 'y', 255, 255, 255);
                        buffstr[i] = 'y';
                        }
                        i++;
                        break;
                case 22:
                        if(use_uppercase) {
                            drawText(mbi, 'U', 255, 255, 255);
                            buffstr[i] = 'U';
                        } else {
                        drawText(mbi, 'u', 255, 255, 255);
                        buffstr[i] = 'u';
                        }
                        i++;
                        break;
                case 23:
                        if(use_uppercase) {
                            drawText(mbi, 'I', 255, 255, 255);
                            buffstr[i] = 'I';
                        } else {
                        drawText(mbi, 'i', 255, 255, 255);
                        buffstr[i] = 'i';
                        }
                        i++;
                        break;
                case 24:
                        if(use_uppercase) {
                            drawText(mbi, 'O', 255, 255, 255);
                            buffstr[i] = 'O';
                        } else {
                        drawText(mbi, 'o', 255, 255, 255);
                        buffstr[i] = 'o';
                        }
                        i++;
                        break;
                case 25:
                        if(use_uppercase) {
                            drawText(mbi, 'P', 255, 255, 255);
                            buffstr[i] = 'P';
                        } else {
                        drawText(mbi, 'p', 255, 255, 255);
                        buffstr[i] = 'p';
                        }
                        i++;
                        break;
                case 26:
                        if(shift_pressed) {
                            drawText(mbi, '{', 255, 255, 255);
                            buffstr[i] = '{';
                        } else {
                        drawText(mbi, '[', 255, 255, 255);
                        buffstr[i] = '[';
                        }
                        i++;
                        break;
                case 27:
                        if(shift_pressed) {
                            drawText(mbi, '}', 255, 255, 255);
                            buffstr[i] = '}';
                        } else {
                        drawText(mbi, ']', 255, 255, 255);
                        buffstr[i] = ']';
                        }
                        i++;
                        break;
                case 28:  // Enter
                        buffstr[i] = '\0';  // Properly terminate the string
                        reading = 0;
                        break;
                case 29:  // Left Control
                        // Handle control key if needed
                        break;
                case 30:
                        if(use_uppercase) {
                            drawText(mbi, 'A', 255, 255, 255);
                            buffstr[i] = 'A';
                        } else {
                        drawText(mbi, 'a', 255, 255, 255);
                        buffstr[i] = 'a';
                        }
                        i++;
                        break;
                case 31:
                        if(use_uppercase) {
                            drawText(mbi, 'S', 255, 255, 255);
                            buffstr[i] = 'S';
                        } else {
                        drawText(mbi, 's', 255, 255, 255);
                        buffstr[i] = 's';
                        }
                        i++;
                        break;
                case 32:
                        if(use_uppercase) {
                            drawText(mbi, 'D', 255, 255, 255);
                            buffstr[i] = 'D';
                        } else {
                        drawText(mbi, 'd', 255, 255, 255);
                        buffstr[i] = 'd';
                        }
                        i++;
                        break;
                case 33:
                        if(use_uppercase) {
                            drawText(mbi, 'F', 255, 255, 255);
                            buffstr[i] = 'F';
                        } else {
                        drawText(mbi, 'f', 255, 255, 255);
                        buffstr[i] = 'f';
                        }
                        i++;
                        break;
                case 34:
                        if(use_uppercase) {
                            drawText(mbi, 'G', 255, 255, 255);
                            buffstr[i] = 'G';
                        } else {
                        drawText(mbi, 'g', 255, 255, 255);
                        buffstr[i] = 'g';
                        }
                        i++;
                        break;
                case 35:
                        if(use_uppercase) {
                            drawText(mbi, 'H', 255, 255, 255);
                            buffstr[i] = 'H';
                        } else {
                        drawText(mbi, 'h', 255, 255, 255);
                        buffstr[i] = 'h';
                        }
                        i++;
                        break;
                case 36:
                        if(use_uppercase) {
                            drawText(mbi, 'J', 255, 255, 255);
                            buffstr[i] = 'J';
                        } else {
                        drawText(mbi, 'j', 255, 255, 255);
                        buffstr[i] = 'j';
                        }
                        i++;
                        break;
                case 37:
                        if(use_uppercase) {
                            drawText(mbi, 'K', 255, 255, 255);
                            buffstr[i] = 'K';
                        } else {
                        drawText(mbi, 'k', 255, 255, 255);
                        buffstr[i] = 'k';
                        }
                        i++;
                        break;
                case 38:
                        if(use_uppercase) {
                            drawText(mbi, 'L', 255, 255, 255);
                            buffstr[i] = 'L';
                        } else {
                        drawText(mbi, 'l', 255, 255, 255);
                        buffstr[i] = 'l';
                        }
                        i++;
                        break;
                case 39:
                        if(shift_pressed) {
                            drawText(mbi, ':', 255, 255, 255);
                            buffstr[i] = ':';
                        } else {
                        drawText(mbi, ';', 255, 255, 255);
                        buffstr[i] = ';';
                        }
                        i++;
                        break;
                case 40:
                        if(shift_pressed) {
                            drawText(mbi, '"', 255, 255, 255);
                            buffstr[i] = '"';
                        } else {
                            drawText(mbi, '\'', 255, 255, 255);
                            buffstr[i] = '\'';
                        }
                        i++;
                        break;
                case 41:
                        if(shift_pressed) {
                            drawText(mbi, '~', 255, 255, 255);
                            buffstr[i] = '~';
                        } else {
                            drawText(mbi, '`', 255, 255, 255);
                            buffstr[i] = '`';
                        }
                        i++;
                        break;
                case 43:
                        if(shift_pressed) {
                            drawText(mbi, '|', 255, 255, 255);
                            buffstr[i] = '|';
                                                        } else {
                            drawText(mbi, '\\', 255, 255, 255);
                            buffstr[i] = '\\';
                        }
                        i++;
                        break;
                case 44:
                        if(use_uppercase) {
                            drawText(mbi, 'Z', 255, 255, 255);
                            buffstr[i] = 'Z';
                        } else {
                        drawText(mbi, 'z', 255, 255, 255);
                        buffstr[i] = 'z';
                        }
                        i++;
                        break;
                case 45:
                        if(use_uppercase) {
                            drawText(mbi, 'X', 255, 255, 255);
                            buffstr[i] = 'X';
                        } else {
                        drawText(mbi, 'x', 255, 255, 255);
                        buffstr[i] = 'x';
                        }
                        i++;
                        break;
                case 46:
                        if(use_uppercase) {
                            drawText(mbi, 'C', 255, 255, 255);
                            buffstr[i] = 'C';
                        } else {
                        drawText(mbi, 'c', 255, 255, 255);
                        buffstr[i] = 'c';
                        }
                        i++;
                        break;
                case 47:
                        if(use_uppercase) {
                            drawText(mbi, 'V', 255, 255, 255);
                            buffstr[i] = 'V';
                        } else {
                        drawText(mbi, 'v', 255, 255, 255);
                        buffstr[i] = 'v';
                        }
                        i++;
                        break;
                case 48:
                        if(use_uppercase) {
                            drawText(mbi, 'B', 255, 255, 255);
                            buffstr[i] = 'B';
                        } else {
                        drawText(mbi, 'b', 255, 255, 255);
                        buffstr[i] = 'b';
                        }
                        i++;
                        break;
                case 49:
                        if(use_uppercase) {
                            drawText(mbi, 'N', 255, 255, 255);
                            buffstr[i] = 'N';
                        } else {
                        drawText(mbi, 'n', 255, 255, 255);
                        buffstr[i] = 'n';
                        }
                        i++;
                        break;
                case 50:
                        if(use_uppercase) {
                            drawText(mbi, 'M', 255, 255, 255);
                            buffstr[i] = 'M';
                        } else {
                        drawText(mbi, 'm', 255, 255, 255);
                        buffstr[i] = 'm';
                        }
                        i++;
                        break;
                case 51:
                        if(shift_pressed) {
                            drawText(mbi, '<', 255, 255, 255);
                            buffstr[i] = '<';
                        } else {
                        drawText(mbi, ',', 255, 255, 255);
                        buffstr[i] = ',';
                        }
                        i++;
                        break;
                case 52:
                        if(shift_pressed) {
                            drawText(mbi, '>', 255, 255, 255);
                            buffstr[i] = '>';
                        } else {
                        drawText(mbi, '.', 255, 255, 255);
                        buffstr[i] = '.';
                        }
                        i++;
                        break;
                case 53:
                        if(shift_pressed) {
                            drawText(mbi, '?', 255, 255, 255);
                            buffstr[i] = '?';
                        } else {
                        drawText(mbi, '/', 255, 255, 255);
                        buffstr[i] = '/';
                        }
                        i++;
                        break;
                case 56:  // Right Alt
                        // Handle right alt if needed
                        break;
                case 57:  // Space
                        drawText(mbi, ' ', 255, 255, 255);
                        buffstr[i] = ' ';
                        i++;
                        break;
                case 69:  // Num Lock
                        // Handle num lock if needed
                        break;
                case 70:  // Scroll Lock
                        // Handle scroll lock if needed
                        break;
                case 71:  // Home
                        // Handle home key if needed
                        break;
                case 72:  // Up Arrow
                        // Handle up arrow if needed
                        break;
                case 73:  // Page Up
                        // Handle page up if needed
                        break;
                case 75:  // Left Arrow
                        // Handle left arrow if needed
                        break;
                case 77:  // Right Arrow
                        // Handle right arrow if needed
                        break;
                case 79:  // End
                        // Handle end key if needed
                        break;
                case 80:  // Down Arrow
                        // Handle down arrow if needed
                        break;
                case 81:  // Page Down
                        // Handle page down if needed
                        break;
                case 82:  // Insert
                        // Handle insert key if needed
                        break;
                case 83:  // Delete
                        // Handle delete key if needed
                        break;
            }
        }
    }
    buffstr[i] = 0;  // Remove the i-1 to keep the last character
    return buffstr;
}