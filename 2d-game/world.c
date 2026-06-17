#include "world.h"
#include "entity_manager.h"  
#include "dungeon_gen.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#define DEFAULT_ENEMIES 5
#define DEFAULT_ITEMS 3

void world_init(World *w, int width, int height) {
    // Initialize world dimensions and clear inventory
    w->width          = width;
    w->height         = height;
    w->inventory_count = 0;               // start with empty inventory

    // Initialize tile memory pool and allocate row pointers
    // Initialize a row-based memory pool with pool_init 
    // Allocate an array of row pointers with malloc, then assign each row’s tile memory via pool_alloc.
    pool_init(&w->tile_pool,
              height,
              width * sizeof(Tile));      // one block per row, each row holds width Tiles
    w->tiles = malloc(sizeof(Tile*) * height);
    if (w->tiles == NULL) {
        fprintf(stderr, "Failed to allocate tile pointers\n");
        return;
    }
    // Assign each row pointer by allocating a block of Tile objects (width tiles) from the pool
    for (int y = 0; y < height; y++) {
        w->tiles[y] = (Tile*)pool_alloc(&w->tile_pool);
    }

    // Generate dungeon
    dungeon_gen(w->tiles, width, height);

    // Initialize entity manager
    entity_manager_init(&w->em, (size_t)width * height);

    // Place the player at the start position (1,1)
    w->player_id = entity_create(&w->em, ENTITY_PLAYER, 1, 1);
    ManagedEntity *player = entity_get(&w->em, w->player_id);
    if (player) {
        // Link the player entity to its tile
        w->tiles[player->y][player->x].occupant = player;
        w->player_hp = player->hp;
    }

// Place DEFAULT_ENEMIES enemies on random empty floor tiles
for (int i = 0; i < DEFAULT_ENEMIES; i++) {
    int x = rand() % width;
    int y = rand() % height;
    // Retry until we find a floor tile with no occupant
    while (w->tiles[y][x].type == TILE_WALL ||
           w->tiles[y][x].occupant != NULL) {
        x = rand() % width;
        y = rand() % height;
    }
    int eid = entity_create(&w->em, ENTITY_ENEMY, x, y);
    w->tiles[y][x].occupant = entity_get(&w->em, eid);
}

// Place DEFAULT_ITEMS items on random empty floor tiles
for (int i = 0; i < DEFAULT_ITEMS; i++) {
    int x = rand() % width;
    int y = rand() % height;
    // Retry until we find a floor tile with no occupant
    while (w->tiles[y][x].type == TILE_WALL ||
           w->tiles[y][x].occupant != NULL) {
        x = rand() % width;
        y = rand() % height;
    }
    int iid = entity_create(&w->em, ENTITY_ITEM, x, y);
    w->tiles[y][x].occupant = entity_get(&w->em, iid);
}
}

// Allocate a new World, initialize it, and return the pointer (or NULL on error)
World* world_create(int width, int height) {
    World *w = malloc(sizeof(World));
    if (w == NULL) {
        fprintf(stderr, "Failed to allocate World struct\n");
        return NULL;
    }
    // Initialize internal structures (tiles, entities, etc.)
    world_init(w, width, height);
    return w;
}

// Free all World resources and then the World struct itself
void world_free(World *w) {
    if (w == NULL) {
        return;
    }
    // Return each tile row to the pool
    for (int y = 0; y < w->height; y++) {
        pool_free(&w->tile_pool, w->tiles[y]);
    }
    // Destroy the tile pool
    pool_destroy(&w->tile_pool);
    // Free the row-pointer array
    free(w->tiles);
    // Destroy all entities
    entity_manager_destroy(&w->em);
    // Free the World struct
    free(w);
}

// Draw the current world grid to the console for visualization
void draw_world(World *w) {
    // Iterate over each row
    for (int y = 0; y < w->height; y++) {
        // Iterate over each column in the row
        for (int x = 0; x < w->width; x++) {
            // If an entity occupies this tile, draw its symbol
            if (w->tiles[y][x].occupant) {
                switch (w->tiles[y][x].occupant->type) {
                    case ENTITY_PLAYER: {
                        putchar('P');  // draw player
                        break;
                    }
                    case ENTITY_ENEMY: {
                        putchar('E');  // draw enemy
                        break;
                    }
                    case ENTITY_ITEM: {
                        putchar('I');  // draw item
                        break;
                    }
                    default: {
                        putchar('?');  // unknown entity
                        break;
                    }
                }
            } else {
                // No occupant: draw wall (#) or empty floor (.) for clarity
                putchar(
                    w->tiles[y][x].type == TILE_WALL
                        ? '#'
                        : '.'
                );
            }
        }
        putchar('\n');  // newline after each row
    }
    // Print legend so player knows what each symbol means
    printf("Legend: P=Player, E=Enemy, I=Item, #=Wall, .=Empty\n");
}

// Move the player in the given direction ('w','a','s','d').
// Returns: 0 = invalid move, 1 = moved, 2 = picked up item
int move_player(World *w, char dir) {
    // Fetch the player entity; fail if not found
    ManagedEntity *p = entity_get(&w->em, w->player_id);
    if (p == NULL) {
        return 0;
    }

    // Determine movement delta
    int dx = 0;
    int dy = 0;
    if (dir == 'w') {
        dy = -1;
    }
    else if (dir == 's') {
        dy = 1;
    }
    else if (dir == 'a') {
        dx = -1;
    }
    else if (dir == 'd') {
        dx = 1;
    }
    else {
        return 0;  // unsupported key
    }

    // Compute new coordinates
    int nx = p->x + dx;
    int ny = p->y + dy;

    // Check bounds
    if (nx < 0 || nx >= w->width || ny < 0 || ny >= w->height) {
        return 0;
    }

    // Check for wall
    if (w->tiles[ny][nx].type == TILE_WALL) {
        return 0;
    }

    // Handle collision with any occupant
    ManagedEntity *occ = w->tiles[ny][nx].occupant;
    if (occ != NULL) {
        if (occ->type == ENTITY_ITEM) {
            // Pick up item if inventory has space
            if (w->inventory_count < DEFAULT_ITEMS) {
                w->inventory[w->inventory_count++] = occ->id;
            }
            // Remove the item entity and clear tile
            entity_delete(&w->em, occ->id);
            w->tiles[ny][nx].occupant = NULL;

            // Move player into the tile
            w->tiles[p->y][p->x].occupant = NULL;
            p->x = nx;
            p->y = ny;
            w->tiles[ny][nx].occupant = p;
            return 2;  // item picked up
        }
        else if (occ->type == ENTITY_ENEMY) {
            // Hit by enemy: decrement HP and report
            // 
            w->player_hp -= 1;
            printf("You were hit! HP: %d/%d\n",
                   w->player_hp,
                   PLAYER_MAX_HP);
            if (w->player_hp <= 0) {
                printf("You died... Game Over.\n");
                exit(0);
            }
            // Remove enemy and clear tile, then continue moving
            entity_delete(&w->em, occ->id);
            w->tiles[ny][nx].occupant = NULL;
        }
    }

    // Normal move: update occupant pointers
    w->tiles[p->y][p->x].occupant = NULL;
    p->x = nx;
    p->y = ny;
    w->tiles[ny][nx].occupant = p;
    return 1;  // moved successfully
}

// Save the current world state to a binary file.
// Returns true on success, false on failure.
bool save_world(const World *w, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }
    // Save world size and player/inventory info
    fwrite(&w->width,  sizeof(int), 1, fp);
    fwrite(&w->height, sizeof(int), 1, fp);
    fwrite(&w->player_hp, sizeof(int), 1, fp);
    fwrite(&w->player_id, sizeof(int), 1, fp);
    fwrite(&w->inventory_count, sizeof(int), 1, fp);
    fwrite(w->inventory, sizeof(int), w->inventory_count, fp);

    // Save tile data (just type, not occupant pointer)
    for (int y = 0; y < w->height; y++) {
        for (int x = 0; x < w->width; x++) {
            uint8_t t = (uint8_t)w->tiles[y][x].type;
            fwrite(&t, sizeof(uint8_t), 1, fp);
        }
    }
    // Save all active entities (ID, type, position, hp, active flag)
    int cnt = entity_count(&w->em);
    fwrite(&cnt, sizeof(int), 1, fp);
    for (size_t i = 0; i < w->em.max_entities; i++) {
        ManagedEntity *e = &w->em.entities[i];
        if (e->active) {
            fwrite(&e->id,     sizeof(int), 1, fp);
            fwrite(&e->type,   sizeof(int), 1, fp);
            fwrite(&e->x,      sizeof(int), 1, fp);
            fwrite(&e->y,      sizeof(int), 1, fp);
            fwrite(&e->hp,     sizeof(int), 1, fp);    
        }
    }
    fclose(fp);
    return true;
}

// Load a saved world from a binary file.
// Returns pointer to new World, or NULL on failure.
World* load_world(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }
    // Read world size
    int width, height;
    if (fread(&width, sizeof(int), 1, fp) != 1 || fread(&height, sizeof(int), 1, fp) != 1) {
        fclose(fp); return NULL;
    }

    // Allocate new world and basic structures
    World *w = world_create(width, height);
    if (!w) {
        fclose(fp);
        return NULL;
    }

    // Load player status and inventory info
    fread(&w->player_hp, sizeof(int), 1, fp);
    fread(&w->player_id, sizeof(int), 1, fp);
    fread(&w->inventory_count, sizeof(int), 1, fp);
    fread(w->inventory, sizeof(int), w->inventory_count, fp);

    // Load tile data (reset occupant pointer)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t t;
            fread(&t, sizeof(uint8_t), 1, fp);
            w->tiles[y][x].type = (TileType)t;
            w->tiles[y][x].occupant = NULL;
        }
    }

    // Rebuild entity manager and all active entities
    entity_manager_destroy(&w->em);
    entity_manager_init(&w->em, (size_t)width * (size_t)height);
    w->player_id = -1;
    int cnt;
    fread(&cnt, sizeof(int), 1, fp);
    int max_id = 0;
    for (int i = 0; i < cnt; i++) {
        int id, type, x, y, hp;
        fread(&id,   sizeof(int), 1, fp);
        fread(&type, sizeof(int), 1, fp);
        fread(&x,    sizeof(int), 1, fp);
        fread(&y,    sizeof(int), 1, fp);
        fread(&hp,   sizeof(int), 1, fp);
        ManagedEntity *e = (ManagedEntity*)pool_alloc(&w->em.pool);
        e->id     = id;
        e->type   = type;
        e->x      = x;
        e->y      = y;
        e->hp     = hp;     
        e->active = 1;
        // Assign entity pointer to tile occupant
        w->tiles[y][x].occupant = e;
        // Track player entity ID for later reference
        if (type == ENTITY_PLAYER) {
            w->player_id = id;
        }
        if (id > max_id) {
            max_id = id;
        }
    }
    // Ensure next_id is unique
    w->em.next_id = max_id + 1;
    fclose(fp);
    return w;
}
