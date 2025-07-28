#include "../../../include/tui.h"
#include "../../../include/shell_command_info.h"
#include "../../../include/vga.h"
#include "../../../include/util.h"
#include <string.h>

#define HELP_TUI_WIDTH 80
#define CMD_LIST_WIDTH 22
#define DESC_WIDTH (HELP_TUI_WIDTH - CMD_LIST_WIDTH - 4) // -4 for separator and borders
#define MAX_VISIBLE 22

void help_tui() {
    extern const shell_command_info_t __start_shellcmds[];
    extern const shell_command_info_t __stop_shellcmds[];
    
    // Count commands
    int cmd_count = 0;
    for (const shell_command_info_t* cmd = __start_shellcmds; cmd < __stop_shellcmds; ++cmd) {
        cmd_count++;
    }
    
    if (cmd_count == 0) {
        printf("No commands available.\n");
        return;
    }
    
    // Create sorted array of command pointers
    const shell_command_info_t** sorted_cmds = (const shell_command_info_t**) my_malloc(cmd_count * sizeof(const shell_command_info_t*));
    if (!sorted_cmds) {
        printf("Error: Memory allocation failed.\n");
        return;
    }
    
    // Fill array with command pointers
    int idx = 0;
    for (const shell_command_info_t* cmd = __start_shellcmds; cmd < __stop_shellcmds; ++cmd) {
        sorted_cmds[idx++] = cmd;
    }
    
    // Sort commands alphabetically by name
    if (cmd_count > 1) {
        for (int i = 0; i < cmd_count - 1; i++) {
            for (int j = 0; j < cmd_count - i - 1; j++) {
                if (strcmp(sorted_cmds[j]->name, sorted_cmds[j + 1]->name) > 0) {
                    const shell_command_info_t* temp = sorted_cmds[j];
                    sorted_cmds[j] = sorted_cmds[j + 1];
                    sorted_cmds[j + 1] = temp;
                }
            }
        }
    }
    
    int selected = 0;
    int scroll = 0;
    int max_visible = MAX_VISIBLE;
    int win_height = max_visible + 3; // title + separator + list + bottom border

    tui_window_t left_win = {0, 0, CMD_LIST_WIDTH, win_height, "Commands", {TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 1}, {TUI_COLOR_GRAY, TUI_COLOR_BLACK, 0}, {TUI_COLOR_BLACK, TUI_COLOR_BLACK, 0}};
    tui_window_t right_win = {CMD_LIST_WIDTH + 2, 0, DESC_WIDTH, win_height, "Description", {TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 1}, {TUI_COLOR_GRAY, TUI_COLOR_BLACK, 0}, {TUI_COLOR_BLACK, TUI_COLOR_BLACK, 0}};

    while (1) {
        tui_clear();
        tui_draw_window(&left_win);
        tui_draw_window(&right_win);

        static char* cmd_names[128];
        for (int i = 0; i < cmd_count; ++i) cmd_names[i] = (char*)sorted_cmds[i]->name;
        tui_style_t norm_style = {TUI_COLOR_WHITE, TUI_COLOR_BLACK, 0};
        tui_style_t sel_style = {TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 1};
        tui_draw_list(&left_win, (const char**)cmd_names, cmd_count, selected, scroll, norm_style, sel_style);

        char desc_buf[256] = "";
        if (sorted_cmds[selected]->description && sorted_cmds[selected]->description[0]) {
            strncat(desc_buf, sorted_cmds[selected]->description, sizeof(desc_buf) - strlen(desc_buf) - 1);
            strncat(desc_buf, "\n", sizeof(desc_buf) - strlen(desc_buf) - 1);
        }
        if (sorted_cmds[selected]->example && sorted_cmds[selected]->example[0]) {
            strncat(desc_buf, "Example: ", sizeof(desc_buf) - strlen(desc_buf) - 1);
            strncat(desc_buf, sorted_cmds[selected]->example, sizeof(desc_buf) - strlen(desc_buf) - 1);
        }
        tui_draw_text_area(&right_win, desc_buf, 0, norm_style);

        tui_style_t status_style = {TUI_COLOR_WHITE, TUI_COLOR_BLACK, 0};
        tui_draw_status_bar(&left_win, "^/v : Move | Ctrl+X: Exit", status_style);

        tui_refresh();
        int key = tui_read_key();
        if (key == 0x1001) { // Up
            if (selected > 0) {
                selected--;
                if (selected < scroll) scroll--;
            }
        } else if (key == 0x1002) { // Down
            if (selected < cmd_count - 1) {
                selected++;
                if (selected >= scroll + max_visible) scroll++;
            }
        } else if (key == 0x2002) { // Ctrl+X
            break;
        } else if (key == '\n' || key == 13) {
            // In future: expand to show more details or usage
        }
    }
    
    // Clean up
    my_free(sorted_cmds);
    printf("\n\n");
} 