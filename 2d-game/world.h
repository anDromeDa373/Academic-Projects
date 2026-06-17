#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include <stdint.h>
#include "db_engine.h"         // MemoryPool 定義
#include "entity_manager.h"    // EntityManager 定義

#define MAX_INVENTORY 3
// Use enum for tile types
// Tile types:
typedef enum {
    TILE_EMPTY,
    TILE_WALL
} TileType;

// Tile represents a single map cell with its type and any occupying entity.
typedef struct {
    TileType        type;      // cell type: TILE_EMPTY or TILE_WALL
    ManagedEntity  *occupant;  // pointer to entity on this tile, or NULL if none
} Tile;

// World represents the game map and overall game state.
typedef struct {
    int           width;            // map width in tiles
    int           height;           // map height in tiles
    Tile        **tiles;            // 2D array of Tile objects
    MemoryPool    tile_pool;        // pool used to allocate tiles
    EntityManager em;               // manager for all entities in the world
    int           player_id;        // ID of the player entity
    int           player_hp;        // current HP of the player
    int           inventory[MAX_INVENTORY];  // list of collected item IDs
    int           inventory_count;  // number of items in inventory
} World;

// Initialize World: allocate memory for tiles, set up map and entities
void world_init(World *w, int width, int height);

// Allocate and initialize a new World, returns pointer or NULL on failure
World* world_create(int width, int height);

// Free all memory used by the World
void world_free(World *w);

// Render the world to the console
void draw_world(World *w);

// Move the player using 'w', 'a', 's', 'd'; return 1 on success, 0 if invalid
int move_player(World *w, char dir);

// Save the World to a binary file; return true on success, false on error
bool save_world(const World *w, const char *filename);

// Load the World from a binary file (allocated with malloc); return pointer or NULL on failure
World* load_world(const char *filename);

#endif 
