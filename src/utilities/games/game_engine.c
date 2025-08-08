#include <game_engine.h>
#include <vga.h>
#include <util.h>
#include <string.h>
#include <eynfs.h>
#include <system.h>
#include <kb.h>
#include <tui.h>
#include <math.h>
#include <stdint.h>

// Global variables
extern uint8 g_current_drive;
#define EYNFS_SUPERBLOCK_LBA 2048

// Game types
#define GAME_TYPE_GENERIC 0
#define GAME_TYPE_SNAKE   1
#define GAME_TYPE_MAZE    2

// Snake-specific data structure
typedef struct {
    uint8_t snake_x[100];     // Snake body X coordinates
    uint8_t snake_y[100];     // Snake body Y coordinates
    uint8_t snake_length;     // Current snake length
    uint8_t direction;        // Current direction (0=up, 1=right, 2=down, 3=left)
    uint8_t food_x;           // Food X coordinate
    uint8_t food_y;           // Food Y coordinate
    uint8_t last_move_time;   // Last move time for continuous movement
} snake_data_t;

// Game engine initialization
int game_engine_init(void) {
    // Initialize TUI system
    tui_init(80, 25);
    return 0;
}

// Game engine cleanup
void game_engine_cleanup(void) {
    // Clean up any game engine resources
    // For now, nothing to clean up
}

// Parse a key-value pair from text
int parse_key_value(const char* line, char* key, char* value) {
    int i = 0, j = 0;
    
    // Skip leading whitespace
    while (line[i] && (line[i] == ' ' || line[i] == '\t')) i++;
    
    // Read key
    while (line[i] && line[i] != ':' && line[i] != '=' && j < 31) {
        key[j++] = line[i++];
    }
    key[j] = '\0';
    
    if (!line[i] || (line[i] != ':' && line[i] != '=')) return -1;
    i++; // Skip : or =
    
    // Skip whitespace after separator
    while (line[i] && (line[i] == ' ' || line[i] == '\t')) i++;
    
    // Read value
    j = 0;
    while (line[i] && line[i] != '\n' && line[i] != '\r' && j < 127) {
        value[j++] = line[i++];
    }
    value[j] = '\0';
    
    return 0;
}

// Parse object definition
int parse_object(const char* line, game_object_t* obj) {
    char key[32], value[128];
    char* token;
    char* rest = (char*)line;
    
    // Skip object name and colon
    while (*rest && *rest != ':') rest++;
    if (*rest) rest++; // Skip colon
    
    memset(obj, 0, sizeof(game_object_t));
    
    while ((token = strtok_r(rest, ",", &rest))) {
        if (parse_key_value(token, key, value) == 0) {
            if (strcmp(key, "type") == 0) obj->type = atoi(value);
            else if (strcmp(key, "symbol") == 0) obj->symbol = value[0];
            else if (strcmp(key, "color") == 0) obj->color = atoi(value);
            else if (strcmp(key, "solid") == 0) obj->solid = atoi(value);
            else if (strcmp(key, "collectible") == 0) obj->collectible = atoi(value);
            else if (strcmp(key, "damage") == 0) obj->damage = atoi(value);
            else if (strcmp(key, "health") == 0) obj->health = atoi(value);
            else if (strcmp(key, "name") == 0) strncpy(obj->name, value, 15);
        }
    }
    
    return 0;
}

// Parse entity definition
int parse_entity(const char* line, game_entity_t* entity) {
    char key[32], value[128];
    char* token;
    char* rest = (char*)line;
    
    // Skip entity name and colon
    while (*rest && *rest != ':') rest++;
    if (*rest) rest++; // Skip colon
    
    memset(entity, 0, sizeof(game_entity_t));
    
    while ((token = strtok_r(rest, ",", &rest))) {
        if (parse_key_value(token, key, value) == 0) {
            if (strcmp(key, "x") == 0) entity->x = atoi(value);
            else if (strcmp(key, "y") == 0) entity->y = atoi(value);
            else if (strcmp(key, "direction") == 0) entity->direction = atoi(value);
            else if (strcmp(key, "symbol") == 0) entity->symbol = value[0];
            else if (strcmp(key, "color") == 0) entity->color = atoi(value);
            else if (strcmp(key, "health") == 0) entity->health = atoi(value);
            else if (strcmp(key, "max_health") == 0) entity->max_health = atoi(value);
            else if (strcmp(key, "damage") == 0) entity->damage = atoi(value);
            else if (strcmp(key, "speed") == 0) entity->speed = atoi(value);
            else if (strcmp(key, "name") == 0) strncpy(entity->name, value, 15);
            else if (strcmp(key, "controlled") == 0) entity->controlled = atoi(value);
            else if (strcmp(key, "ai_type") == 0) entity->ai_type = atoi(value);
        }
    }
    
    return 0;
}

// Read game text file from EYNFS
int game_read_text_file(const char* filename, char** buffer, int* size) {
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return -1;
    }
    
    // Find the game file
    eynfs_dir_entry_t entry;
    uint32_t parent_block, entry_index;
    if (eynfs_traverse_path(g_current_drive, &sb, filename, &entry, &parent_block, &entry_index) != 0) {
        printf("%cError: Game file not found: %s\n", 255, 0, 0, filename);
        return -1;
    }
    
    if (entry.type != EYNFS_TYPE_FILE) {
        printf("%cError: %s is not a file.\n", 255, 0, 0, filename);
        return -1;
    }
    
    // Allocate memory for file content
    *buffer = (char*)my_malloc(entry.size + 1);
    if (!*buffer) {
        printf("%cError: Failed to allocate memory for game file.\n", 255, 0, 0);
        return -1;
    }
    
    // Read the file content
    int bytes_read = eynfs_read_file(g_current_drive, &sb, &entry, *buffer, entry.size, 0);
    if (bytes_read != entry.size) {
        printf("%cError: Failed to read game file.\n", 255, 0, 0);
        my_free(*buffer);
        *buffer = NULL;
        return -1;
    }
    
    (*buffer)[entry.size] = '\0'; // Null terminate
    *size = entry.size;
    
    return 0;
}

// Parse game text file
int game_parse_text_file(const char* filename, game_config_t** config) {
    char* buffer;
    int size;
    
    if (game_read_text_file(filename, &buffer, &size) != 0) {
        return -1;
    }
    
    // Allocate config
    *config = (game_config_t*)my_malloc(sizeof(game_config_t));
    if (!*config) {
        my_free(buffer);
        return -1;
    }
    
    memset(*config, 0, sizeof(game_config_t));
    
    // Parse file line by line
    char* saveptr;
    char* line = strtok_r(buffer, "\n", &saveptr);
    int in_level = 0, in_objects = 0, in_entities = 0, in_layout = 0;
    int level_index = 0;
    int object_index = 0;
    int entity_index = 0;
    int layout_y = 0;
    
    while (line) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0' || line[0] == '\r') {
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }
        
        // Check for section headers
        if (strncmp(line, "[level]", 7) == 0) {
            in_level = 1;
            in_objects = 0;
            in_entities = 0;
            in_layout = 0;
            
            // Allocate new level
            if (level_index == 0) {
                (*config)->levels = (game_level_t*)my_malloc(sizeof(game_level_t));
                } else {
                (*config)->levels = (game_level_t*)my_realloc((*config)->levels, 
                    (level_index + 1) * sizeof(game_level_t));
            }
            
            memset(&(*config)->levels[level_index], 0, sizeof(game_level_t));
            object_index = 0;
            entity_index = 0;
            layout_y = 0;
        }
        else if (strncmp(line, "[objects]", 9) == 0) {
            in_objects = 1;
            in_entities = 0;
            in_layout = 0;
        }
        else if (strncmp(line, "[entities]", 10) == 0) {
            in_entities = 1;
            in_objects = 0;
            in_layout = 0;
        }
        else if (strncmp(line, "[layout]", 8) == 0) {
            in_layout = 1;
            in_objects = 0;
            in_entities = 0;
        }
        else if (strncmp(line, "[rules]", 7) == 0) {
            // Skip rules section for now
            in_level = 0;
            in_objects = 0;
            in_entities = 0;
            in_layout = 0;
        }
        else if (in_level && in_objects && strchr(line, ':')) {
            // Parse object definition
            if (object_index == 0) {
                (*config)->levels[level_index].objects = (game_object_t*)my_malloc(sizeof(game_object_t));
            } else {
                (*config)->levels[level_index].objects = (game_object_t*)my_realloc(
                    (*config)->levels[level_index].objects, 
                    (object_index + 1) * sizeof(game_object_t));
            }
            
            parse_object(line, &(*config)->levels[level_index].objects[object_index]);
            object_index++;
            (*config)->levels[level_index].object_count = object_index;
        }
        else if (in_level && in_entities && strchr(line, ':')) {
            // Parse entity definition
            if (entity_index == 0) {
                (*config)->levels[level_index].entities = (game_entity_t*)my_malloc(sizeof(game_entity_t));
            } else {
                (*config)->levels[level_index].entities = (game_entity_t*)my_realloc(
                    (*config)->levels[level_index].entities, 
                    (entity_index + 1) * sizeof(game_entity_t));
            }
            
            parse_entity(line, &(*config)->levels[level_index].entities[entity_index]);
            
            // Check if this is the player
            if (strstr(line, "controlled=1")) {
                (*config)->levels[level_index].player_entity = entity_index;
            }
            
            entity_index++;
            (*config)->levels[level_index].entity_count = entity_index;
        }
        else if (in_level && in_layout && line[0] >= '0' && line[0] <= '9') {
            // Parse layout line
            int width = strlen(line);
            if (layout_y == 0) {
                (*config)->levels[level_index].width = width;
                (*config)->levels[level_index].board = (uint8_t*)my_malloc(width * 20); // Assume max height 20
            }
            
            for (int x = 0; x < width && x < (*config)->levels[level_index].width; x++) {
                (*config)->levels[level_index].board[layout_y * width + x] = line[x] - '0';
            }
            layout_y++;
            (*config)->levels[level_index].height = layout_y;
        }
        else if (!in_level && strchr(line, ':')) {
            // Parse global config
            char key[32], value[128];
            if (parse_key_value(line, key, value) == 0) {
                if (strcmp(key, "title") == 0) strncpy((*config)->title, value, 31);
                else if (strcmp(key, "description") == 0) strncpy((*config)->description, value, 127);
                else if (strcmp(key, "controls") == 0) strncpy((*config)->controls, value, 63);
                else if (strcmp(key, "board_width") == 0) (*config)->board_width = atoi(value);
                else if (strcmp(key, "board_height") == 0) (*config)->board_height = atoi(value);
                else if (strcmp(key, "game_speed") == 0) (*config)->game_speed = atoi(value);
                else if (strcmp(key, "max_entities") == 0) (*config)->max_entities = atoi(value);
                else if (strcmp(key, "max_objects") == 0) (*config)->max_objects = atoi(value);
                else if (strcmp(key, "start_level") == 0) (*config)->start_level = atoi(value);
            }
        }
        
        line = strtok_r(NULL, "\n", &saveptr);
    }
    
    (*config)->level_count = level_index + 1;
    my_free(buffer);
    
    return 0;
}

// Free game configuration
void game_free_config(game_config_t* config) {
    if (!config) return;
    
    for (int i = 0; i < config->level_count; i++) {
        if (config->levels[i].board) my_free(config->levels[i].board);
        if (config->levels[i].objects) my_free(config->levels[i].objects);
        if (config->levels[i].entities) my_free(config->levels[i].entities);
    }
    
    if (config->levels) my_free(config->levels);
    my_free(config);
}

// Free game state
void game_free_state(game_state_t* state) {
    if (!state) return;
    
    if (state->display_buffer) my_free(state->display_buffer);
    if (state->game_specific_data) my_free(state->game_specific_data);
    game_free_config(state->config);
}

// Determine game type from title
int get_game_type(const char* title) {
    if (strstr(title, "Snake") || strstr(title, "snake")) {
        return GAME_TYPE_SNAKE;
    } else if (strstr(title, "Maze") || strstr(title, "maze")) {
        return GAME_TYPE_MAZE;
    }
    return GAME_TYPE_GENERIC;
}

// Initialize snake game
void init_snake_game(game_state_t* state) {
    if (!state || !state->current_level) return;
    
    // Allocate snake data
    snake_data_t* snake_data = (snake_data_t*)my_malloc(sizeof(snake_data_t));
    if (!snake_data) return;
    
    // Initialize snake
    snake_data->snake_length = 3;
    snake_data->direction = 1; // Start moving right
    
    // Place snake in center
    int center_x = state->current_level->width / 2;
    int center_y = state->current_level->height / 2;
    
    snake_data->snake_x[0] = center_x;
    snake_data->snake_y[0] = center_y;
    snake_data->snake_x[1] = center_x - 1;
    snake_data->snake_y[1] = center_y;
    snake_data->snake_x[2] = center_x - 2;
    snake_data->snake_y[2] = center_y;
    
    // Place initial food
    snake_data->food_x = center_x + 3;
    snake_data->food_y = center_y;
    snake_data->last_move_time = 0;
    
    // Store snake data in state
    state->game_specific_data = snake_data;
}

// Move snake (snake-specific logic)
void move_snake(game_state_t* state) {
    if (!state || !state->current_level || !state->game_specific_data) return;
    
    snake_data_t* snake_data = (snake_data_t*)state->game_specific_data;

    // Calculate new head position
    int head_x = snake_data->snake_x[0];
    int head_y = snake_data->snake_y[0];
    
    switch (snake_data->direction) {
        case 0: head_y--; break; // Up
        case 1: head_x++; break; // Right
        case 2: head_y++; break; // Down
        case 3: head_x--; break; // Left
    }

    // Check for wall collision
    if (head_x < 0 || head_x >= state->current_level->width || 
        head_y < 0 || head_y >= state->current_level->height) {
        state->game_over = 1;
        return;
    }

    // Check for self collision
    for (int i = 0; i < snake_data->snake_length; i++) {
        if (snake_data->snake_x[i] == head_x && snake_data->snake_y[i] == head_y) {
            state->game_over = 1;
            return;
        }
    }

    // Move body
    for (int i = snake_data->snake_length - 1; i > 0; i--) {
        snake_data->snake_x[i] = snake_data->snake_x[i - 1];
        snake_data->snake_y[i] = snake_data->snake_y[i - 1];
    }
    snake_data->snake_x[0] = head_x;
    snake_data->snake_y[0] = head_y;

    // Check for food
    if (head_x == snake_data->food_x && head_y == snake_data->food_y) {
        // Grow snake
        if (snake_data->snake_length < 100) {
            snake_data->snake_length++;
            snake_data->snake_x[snake_data->snake_length - 1] = snake_data->snake_x[snake_data->snake_length - 2];
            snake_data->snake_y[snake_data->snake_length - 1] = snake_data->snake_y[snake_data->snake_length - 2];
        }
        state->score++;
        
        // Place new food
        int attempts = 0;
        do {
            snake_data->food_x = rand_range(0, state->current_level->width - 1);
            snake_data->food_y = rand_range(0, state->current_level->height - 1);
            attempts++;
            
            // Check if food is not on snake
            int food_on_snake = 0;
            for (int i = 0; i < snake_data->snake_length; i++) {
                if (snake_data->snake_x[i] == snake_data->food_x && snake_data->snake_y[i] == snake_data->food_y) {
                    food_on_snake = 1;
                    break;
                }
            }
            
            if (!food_on_snake) break;
        } while (attempts < 100);
    }
}

// Check collision at position
int game_check_collision(game_state_t* state, int x, int y) {
    if (!state || !state->current_level || x < 0 || y < 0 || 
        x >= state->current_level->width || y >= state->current_level->height) {
        return 1; // Out of bounds = collision
    }
    
    // Check level layout
    uint8_t cell = state->current_level->board[y * state->current_level->width + x];
    if (cell == 1) return 1; // Wall collision
    
    // Check entities
    for (int i = 0; i < state->current_level->entity_count; i++) {
        if (state->current_level->entities[i].x == x && 
            state->current_level->entities[i].y == y) {
            return 1; // Entity collision
        }
    }
    
    return 0; // No collision
}

// Move entity
int game_move_entity(game_state_t* state, uint8_t entity_index, int dx, int dy) {
    if (!state || !state->current_level || entity_index >= state->current_level->entity_count) {
        return -1;
    }
    
    game_entity_t* entity = &state->current_level->entities[entity_index];
    int new_x = entity->x + dx;
    int new_y = entity->y + dy;
    
    // Check collision
    if (game_check_collision(state, new_x, new_y)) {
        return -1; // Cannot move
    }
    
    // Move entity
    entity->x = new_x;
    entity->y = new_y;
    
    return 0;
}

// Update entities (AI, etc.)
void game_update_entities(game_state_t* state) {
    if (!state || !state->current_level) return;
    
    for (int i = 0; i < state->current_level->entity_count; i++) {
        game_entity_t* entity = &state->current_level->entities[i];
        
        if (!entity->controlled) {
            // Simple AI: random movement
            int dx = 0, dy = 0;
            int direction = rand_range(0, 3);
            
            switch (direction) {
                case 0: dy = -1; break; // Up
                case 1: dx = 1; break;  // Right
                case 2: dy = 1; break;  // Down
                case 3: dx = -1; break; // Left
            }
            
            game_move_entity(state, i, dx, dy);
        }
    }
}

// Handle collisions
void game_handle_collisions(game_state_t* state) {
    if (!state || !state->current_level) return;
    
    // Check player collisions with objects
    game_entity_t* player = &state->current_level->entities[state->current_level->player_entity];
    
    // Check for exit
    uint8_t cell = state->current_level->board[player->y * state->current_level->width + player->x];
    if (cell == 8) { // Exit
        state->won = 1;
        return;
    }
    
    // Check for collectibles
    if (cell == 3 || cell == 7) { // Food or Key
        state->score += 10;
        state->current_level->board[player->y * state->current_level->width + player->x] = 0;
    }
    
    // Check entity collisions
    for (int i = 0; i < state->current_level->entity_count; i++) {
        if (i == state->current_level->player_entity) continue;
        game_entity_t* entity = &state->current_level->entities[i];
        game_entity_t* player = &state->current_level->entities[state->current_level->player_entity];
        if (entity->x == player->x && entity->y == player->y && entity->x < 255 && entity->y < 255) {
            // Combat: both take damage
            if (player->health > 0 && entity->health > 0) {
                player->health--;
                entity->health--;
                if (entity->health <= 0) {
                    state->score += 50;
                    // Remove dead enemy
                    entity->x = 255;
                    entity->y = 255;
                }
                if (player->health <= 0) {
                    state->game_over = 1;
                }
            }
        }
    }
}

// Load game from text file
int game_load_from_text_file(const char* filename, game_state_t* state) {
    if (!state) return -1;
    
    // Initialize state
    memset(state, 0, sizeof(game_state_t));
    
    // Parse game configuration
    if (game_parse_text_file(filename, &state->config) != 0) {
        return -1;
    }
    
    // Set current level
    state->current_level_index = state->config->start_level;
    state->current_level = &state->config->levels[state->current_level_index];
    
    // Allocate display buffer
    int buffer_size = state->current_level->width * state->current_level->height;
    state->display_buffer = (uint8_t*)my_malloc(buffer_size);
    if (!state->display_buffer) {
        game_free_config(state->config);
        state->config = NULL;
        return -1;
    }
    
    // Initialize display buffer from level layout
    memcpy(state->display_buffer, state->current_level->board, buffer_size);
    
    // Initialize game-specific logic
    int game_type = get_game_type(state->config->title);
    if (game_type == GAME_TYPE_SNAKE) {
        // If no layout or layout is all zeros, create a default empty field with borders
        int w = state->current_level->width;
        int h = state->current_level->height;
        int needs_default = 0;
        if (!state->current_level->board) {
            needs_default = 1;
        } else {
            // Check if all cells are 0
            int all_zero = 1;
            for (int i = 0; i < w * h; i++) {
                if (state->current_level->board[i] != 0) { all_zero = 0; break; }
            }
            if (all_zero) needs_default = 1;
        }
        if (needs_default) {
            if (state->current_level->board) my_free(state->current_level->board);
            state->current_level->board = (uint8_t*)my_malloc(w * h);
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    if (y == 0 || y == h-1 || x == 0 || x == w-1) {
                        state->current_level->board[y * w + x] = 1; // Wall
            } else {
                        state->current_level->board[y * w + x] = 0;
                    }
                }
            }
        }
        // Remove all entities for Snake (no generic player)
        state->current_level->entity_count = 0;
        init_snake_game(state);
    }
    
    return 0;
}

// Draw the game board using TUI
void game_draw_board(game_state_t* state) {
    if (!state || !state->config || !state->current_level || !state->display_buffer) return;
    
    // Clear screen
    clearScreen();
    
    // Calculate TUI dimensions
    int screen_width = 80;
    int screen_height = 25;
    int game_width = state->current_level->width + 2;
    int game_height = state->current_level->height + 2;
    
    // Center the game window
    int window_x = (screen_width - game_width) / 2;
    int window_y = (screen_height - game_height) / 2;
    
    // Define styles
    tui_style_t border_style = {TUI_COLOR_GRAY, TUI_COLOR_BLACK, 0};
    tui_style_t title_style = {TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 0};
    tui_style_t status_style = {TUI_COLOR_WHITE, TUI_COLOR_BLACK, 0};
    tui_style_t score_style = {TUI_COLOR_MAGENTA, TUI_COLOR_BLACK, 0};
    
    // Draw title
    char title_text[64];
    snprintf(title_text, sizeof(title_text), "EYN-OS %s Game", state->config->title);
    int title_x = (screen_width - strlen(title_text)) / 2;
    tui_draw_text(title_x, 1, title_text, title_style);
    
    // Draw pause indicator
    if (state->paused) {
        tui_style_t pause_style = {TUI_COLOR_RED, TUI_COLOR_BLACK, 0};
        tui_draw_text(screen_width - 8, 1, "[PAUSED]", pause_style);
    }
    
    // Draw top border (dashes) for Snake, aligned with the window title, but not overlapping it
    if (state->game_specific_data) {
        int border_y = window_y;
        char top_border[game_width + 1];
        for (int i = 0; i < game_width; i++) top_border[i] = '-';
        top_border[game_width] = '\0';
        tui_draw_text(window_x, border_y, top_border, border_style);
    }
    
    // Draw game board
    for (int y = 0; y < state->current_level->height; y++) {
        // Draw left border
        tui_draw_text(window_x, window_y + y + 1, "|", border_style);
        
        // Draw game content
        for (int x = 0; x < state->current_level->width; x++) {
            uint8_t cell = state->current_level->board[y * state->current_level->width + x];
            char cell_char[2] = {' ', '\0'};
            tui_style_t cell_style = border_style;
            
            // Determine cell appearance
            switch (cell) {
                case 0: cell_char[0] = ' '; break; // Empty
                case 1: cell_char[0] = '#'; break; // Wall
                case 3: cell_char[0] = '*'; cell_style = (tui_style_t){TUI_COLOR_RED, TUI_COLOR_BLACK, 0}; break; // Food
                case 6: cell_char[0] = 'D'; cell_style = (tui_style_t){TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 0}; break; // Door
                case 7: cell_char[0] = 'K'; cell_style = (tui_style_t){TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 0}; break; // Key
                case 8: cell_char[0] = 'E'; cell_style = (tui_style_t){TUI_COLOR_MAGENTA, TUI_COLOR_BLACK, 0}; break; // Exit
                default: cell_char[0] = '?'; break;
            }
            
            tui_draw_text(window_x + x + 1, window_y + y + 1, cell_char, cell_style);
        }
        
        // Draw right border
        tui_draw_text(window_x + game_width - 1, window_y + y + 1, "|", border_style);
    }
    
    // Draw bottom border
    char bottom_border[game_width + 1];
    bottom_border[0] = '+';
    for (int i = 1; i < game_width - 1; i++) {
        bottom_border[i] = '-';
    }
    bottom_border[game_width - 1] = '+';
    bottom_border[game_width] = '\0';
    tui_draw_text(window_x, window_y + game_height - 1, bottom_border, border_style);
    
    // Draw entities
    // Only draw entities if not Snake
    if (!state->game_specific_data) {
        for (int i = 0; i < state->current_level->entity_count; i++) {
            game_entity_t* entity = &state->current_level->entities[i];
            if (entity->x < 255 && entity->y < 255) { // Not removed
                char entity_char[2] = {entity->symbol, '\0'};
                tui_style_t entity_style = {entity->color, TUI_COLOR_BLACK, 0};
                tui_draw_text(window_x + entity->x + 1, window_y + entity->y + 1, entity_char, entity_style);
            }
        }
    }
    
    // Draw snake (if snake game)
    if (state->game_specific_data) {
        snake_data_t* snake_data = (snake_data_t*)state->game_specific_data;
        for (int i = 0; i < snake_data->snake_length; i++) {
            char snake_char[2] = {(i == 0) ? '@' : 'O', '\0'};
            tui_style_t snake_style = {TUI_COLOR_YELLOW, TUI_COLOR_BLACK, 0};
            tui_draw_text(window_x + snake_data->snake_x[i] + 1, window_y + snake_data->snake_y[i] + 1, snake_char, snake_style);
        }
        
    // Draw food
        char food_char[2] = {'*', '\0'};
        tui_style_t food_style = {TUI_COLOR_RED, TUI_COLOR_BLACK, 0};
        tui_draw_text(window_x + snake_data->food_x + 1, window_y + snake_data->food_y + 1, food_char, food_style);
    }
    
    // Draw separator line
    for (int x = 0; x < screen_width; x++) {
        tui_draw_text(x, screen_height - 3, "-", border_style);
    }
    
    // Draw status bar
    char status_text[128];
    if (state->game_specific_data) {
        // Snake game - show length instead of health
        snake_data_t* snake_data = (snake_data_t*)state->game_specific_data;
        snprintf(status_text, sizeof(status_text), "Score: %d | Length: %d | %s | Level: %s", 
                 state->score, snake_data->snake_length, state->config->controls, state->current_level->name);
    } else {
        // Other games - show health
        snprintf(status_text, sizeof(status_text), "Score: %d | Health: %d/%d | %s | Level: %s", 
                 state->score, 
                 state->current_level->entities[state->current_level->player_entity].health,
                 state->current_level->entities[state->current_level->player_entity].max_health,
                 state->config->controls, 
                 state->current_level->name);
    }
    
    // Draw status bar with colored score
    char score_part[32];
    snprintf(score_part, sizeof(score_part), "Score: %d", state->score);
    tui_draw_text(0, screen_height - 2, score_part, score_style);
    
    // Draw rest of status bar
    char rest_part[128];
    if (state->game_specific_data) {
        snake_data_t* snake_data = (snake_data_t*)state->game_specific_data;
        snprintf(rest_part, sizeof(rest_part), " | Length: %d | %s | Level: %s", 
                 snake_data->snake_length, state->config->controls, state->current_level->name);
    } else {
        snprintf(rest_part, sizeof(rest_part), " | Health: %d/%d | %s | Level: %s", 
                 state->current_level->entities[state->current_level->player_entity].health,
                 state->current_level->entities[state->current_level->player_entity].max_health,
                 state->config->controls, 
                 state->current_level->name);
    }
    tui_draw_text(strlen(score_part), screen_height - 2, rest_part, status_style);
    
    // Draw game over or win message
    if (state->game_over) {
        tui_style_t game_over_style = {TUI_COLOR_RED, TUI_COLOR_BLACK, 0};
        char game_over_text[64];
        snprintf(game_over_text, sizeof(game_over_text), "GAME OVER! Final Score: %d\n\n", state->score);
        int game_over_x = (screen_width - strlen(game_over_text)) / 2;
        tui_draw_text(game_over_x, screen_height - 4, game_over_text, game_over_style);
    } else if (state->won) {
        tui_style_t win_style = {TUI_COLOR_MAGENTA, TUI_COLOR_BLACK, 0};
        char win_text[64];
        snprintf(win_text, sizeof(win_text), "YOU WIN! Final Score: %d\n\n", state->score);
        int win_x = (screen_width - strlen(win_text)) / 2;
        tui_draw_text(win_x, screen_height - 4, win_text, win_style);
    }
}

// Check if a key is currently pressed (non-blocking)
int game_check_key_pressed() {
    if (inportb(0x64) & 0x1) {
        uint8_t scancode = inportb(0x60);
        if (!(scancode & 0x80)) { // Key press, not release
            return scancode;
        }
    }
    return -1; // No key pressed
}

// Handle game input (non-blocking)
int game_handle_input(game_state_t* state) {
    if (!state || !state->current_level) return 0;
    
    int key = game_check_key_pressed();
    if (key == -1) return 0; // No key pressed
    
    int input_processed = 0;
    
    // Handle snake-specific input
    if (state->game_specific_data) {
        snake_data_t* snake_data = (snake_data_t*)state->game_specific_data;
        
        switch (key) {
            case 17: // W key
            case 72: // Up arrow
                if (snake_data->direction != 2) {
                    snake_data->direction = 0;
                    input_processed = 1;
                }
                break;
            case 32: // D key
            case 77: // Right arrow
                if (snake_data->direction != 3) {
                    snake_data->direction = 1;
                    input_processed = 1;
                }
                break;
            case 31: // S key
            case 80: // Down arrow
                if (snake_data->direction != 0) {
                    snake_data->direction = 2;
                    input_processed = 1;
                }
                break;
            case 30: // A key
            case 75: // Left arrow
                if (snake_data->direction != 1) {
                    snake_data->direction = 3;
                    input_processed = 1;
                }
                break;
        }
    } else {
        // Handle generic game input
        game_entity_t* player = &state->current_level->entities[state->current_level->player_entity];
        
        switch (key) {
            case 17: // W key
            case 72: // Up arrow
                if (game_move_entity(state, state->current_level->player_entity, 0, -1) == 0) {
                    input_processed = 1;
                }
                break;
            case 32: // D key
            case 77: // Right arrow
                if (game_move_entity(state, state->current_level->player_entity, 1, 0) == 0) {
                    input_processed = 1;
                }
                break;
            case 31: // S key
            case 80: // Down arrow
                if (game_move_entity(state, state->current_level->player_entity, 0, 1) == 0) {
                    input_processed = 1;
                }
                break;
            case 30: // A key
            case 75: // Left arrow
                if (game_move_entity(state, state->current_level->player_entity, -1, 0) == 0) {
                    input_processed = 1;
                }
                break;
        }
    }
    
    // Common controls
    switch (key) {
        case 16: // Q key
            return -1; // Quit game
        case 25: // P key
            state->paused = !state->paused;
            input_processed = 1;
            break;
    }
    
    return input_processed;
}

// Main game loop
int game_run(game_state_t* state) {
    if (!state || !state->config) return -1;
    
    printf("%cStarting game: %s\n", 255, 255, 255, state->config->title);
    
    // Game timing variables
    int last_update_time = 0;
    int current_time = 0;
    int needs_redraw = 1; // Force initial draw
    
    // Initial draw
    game_draw_board(state);
    
    while (!state->game_over && !state->won) {
        // Process input immediately
        int input_result = game_handle_input(state);
        if (input_result == -1) {
            break; // Quit game
        }
        
        // Update game state at controlled intervals
        current_time += 1;
        if (current_time - last_update_time >= state->config->game_speed) {
            if (!state->paused) {
                if (state->game_specific_data) {
                    // Snake game - continuous movement
                move_snake(state);
                } else {
                    // Generic game - entity updates
                    game_update_entities(state);
                    game_handle_collisions(state);
                }
                needs_redraw = 1;
            }
            last_update_time = current_time;
        }
        
        // Only redraw when necessary
        if (needs_redraw || input_result == 1) {
            game_draw_board(state);
            needs_redraw = 0;
        }
        
        // Minimal delay for input responsiveness
        sleep(1);
    }
    
    // Only print the text message if neither game_over nor won was already shown in TUI
    // (But since TUI always shows it, just skip the printf entirely)
    
    // Add extra newlines to push shell prompt below status bar
    printf("\n\n");
    
    return 0;
} 