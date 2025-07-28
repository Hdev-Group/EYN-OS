#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "types.h"
#include "tui.h"
#include <stdint.h>

// Game object types
#define GAME_OBJ_EMPTY     0
#define GAME_OBJ_WALL      1
#define GAME_OBJ_PLAYER    2
#define GAME_OBJ_FOOD      3
#define GAME_OBJ_ENEMY     4
#define GAME_OBJ_COLLECTIBLE 5
#define GAME_OBJ_DOOR      6
#define GAME_OBJ_KEY       7
#define GAME_OBJ_EXIT      8
#define GAME_OBJ_CUSTOM    9

// Game object structure
typedef struct {
    uint8_t type;           // Object type
    uint8_t x, y;           // Position
    uint8_t symbol;         // Display symbol (ASCII)
    uint8_t color;          // Display color
    uint8_t solid;          // Is object solid (blocks movement)
    uint8_t collectible;    // Can be collected
    uint8_t damage;         // Damage value (for enemies)
    uint8_t health;         // Health value
    char name[16];          // Object name
} game_object_t;

// Game entity structure (player, enemies, etc.)
typedef struct {
    uint8_t x, y;           // Position
    uint8_t direction;      // Direction (0=up, 1=right, 2=down, 3=left)
    uint8_t symbol;         // Display symbol
    uint8_t color;          // Display color
    uint8_t health;         // Health
    uint8_t max_health;     // Maximum health
    uint8_t damage;         // Attack damage
    uint8_t speed;          // Movement speed
    char name[16];          // Entity name
    uint8_t controlled;     // Is player controlled
    uint8_t ai_type;        // AI behavior type
} game_entity_t;

// Game level structure
typedef struct {
    char name[32];          // Level name
    uint8_t width, height;  // Level dimensions
    uint8_t* board;         // Level layout (object types)
    game_object_t* objects; // Object definitions
    uint8_t object_count;   // Number of objects
    game_entity_t* entities; // Entities in level
    uint8_t entity_count;   // Number of entities
    uint8_t player_entity;  // Index of player entity
} game_level_t;

// Game configuration structure
typedef struct {
    char title[32];         // Game title
    char description[128];  // Game description
    char controls[64];      // Control instructions
    uint8_t board_width;    // Default board width
    uint8_t board_height;   // Default board height
    uint8_t game_speed;     // Game speed (ms per frame)
    uint8_t max_entities;   // Maximum entities
    uint8_t max_objects;    // Maximum objects
    uint8_t start_level;    // Starting level index
    game_level_t* levels;   // Game levels
    uint8_t level_count;    // Number of levels
} game_config_t;

// Game state structure
typedef struct {
    game_config_t* config;  // Game configuration
    game_level_t* current_level; // Current level
    uint8_t current_level_index; // Current level index
    int score;              // Current score
    int game_over;          // Game over flag
    int paused;             // Pause flag
    int won;                // Win flag
    uint8_t* display_buffer; // Display buffer
    void* game_specific_data; // Game-specific data (snake_data_t, etc.)
} game_state_t;

// Game engine function prototypes
int game_engine_init(void);
void game_engine_cleanup(void);
int game_load_from_text_file(const char* filename, game_state_t* state);
int game_run(game_state_t* state);
void game_draw_board(game_state_t* state);
int game_handle_input(game_state_t* state);

// Game data file functions
int game_parse_text_file(const char* filename, game_config_t** config);
void game_free_config(game_config_t* config);
void game_free_state(game_state_t* state);

// Game logic functions
int game_move_entity(game_state_t* state, uint8_t entity_index, int dx, int dy);
int game_check_collision(game_state_t* state, int x, int y);
void game_update_entities(game_state_t* state);
void game_handle_collisions(game_state_t* state);

#endif // GAME_ENGINE_H 