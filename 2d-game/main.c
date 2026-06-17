#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "world.h"

#define DEFAULT_WIDTH    20
#define DEFAULT_HEIGHT   20
#define INPUT_BUF_SIZE   128

int main(int argc, char* argv[]) {
    const char* filepath = NULL;
    int width = DEFAULT_WIDTH;
    int height = DEFAULT_HEIGHT;
    bool new_world = false;
    World* world = NULL;

    // Parse command-line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <world_file> [width height]\n", argv[0]);
        return 1;
    }
    filepath = argv[1];
    // If width and height are provided, create new world; otherwise, try to load world from file
    if (argc >= 4) {
        width  = atoi(argv[2]);
        height = atoi(argv[3]);
        world = world_create(width, height);
        new_world = true;
    } else {
        world = load_world(filepath);
        if (world) {
            new_world = false;
        } else {
            world = world_create(width, height);
            new_world = true;
        }
    }
    if (!world) {
        fprintf(stderr, "Failed to initialize world.\n");
        return 1;
    }

    // Print initial game status
    if (new_world) {
        printf("=== GAME WORLD INITIALIZED ===\n");
        printf("World size: %dx%d\n", width, height);
    } else {
        printf("=== GAME LOADED ===\n");
        printf("World restored from: %s\n", filepath);
    }
    ManagedEntity *p = entity_get(&world->em, world->player_id);
    if (p) {
        printf("Player position: (%d,%d)\n\n", p->x, p->y);
    }
    draw_world(world);

    // Main input loop
    char buf[INPUT_BUF_SIZE];
    while (true) {
        printf("Input: ");
        if (!fgets(buf, sizeof(buf), stdin)) {
            break;
        }
        buf[strcspn(buf, "\r\n")] = '\0';
        // Command: quit the game
        if (strcmp(buf, "quit") == 0) {
            break;
        }
        // Command: save world to file
        else if (strncmp(buf, "save ", 5) == 0) {
            const char* out = buf + 5;
            if (strlen(out) > 0 && save_world(world, out)) {
                struct stat st;
                if (stat(out, &st) == 0) {
                    // Get file size in kilobytes
                    double size_kb = st.st_size / 1024.0;
                    // Count the number of entities saved
                    int cnt = entity_count(&world->em);
                    // Get player entity for position display
                    ManagedEntity *pp = entity_get(&world->em, world->player_id);
                    printf("=== GAME SAVED ===\n");
                    printf("World state saved to: %s\n", out);
                    printf("File size: %.1f KB\n", size_kb);
                    printf("Entities saved: %d\n", cnt);
                    if (pp) printf("Player position: (%d,%d)\n\n", pp->x, pp->y);
                } else {
                    printf("=== GAME SAVED ===\n");
                    printf("World state saved to: %s (size unknown)\n\n", out);
                }
            } else {
                printf("Usage: save <filename> or save failed\n");
            }
        }
        // Command: load world from file
        else if (strncmp(buf, "load ", 5) == 0) {
            const char* in = buf + 5;
            World* nw = load_world(in);
            if (nw) {
                // Free the previous world and switch to the newly loaded one
                world_free(world);
                world = nw;
                // Get player entity for position display
                ManagedEntity *pp = entity_get(&world->em, world->player_id);
                int cnt = entity_count(&world->em);
                printf("=== GAME LOADED ===\n");
                printf("World restored from: %s\n", in);
                if (pp) printf("Player position: (%d,%d)\n", pp->x, pp->y);
                printf("Entities loaded: %d\n\n", cnt);
                draw_world(world);
            } else {
                printf("Failed to load from: %s\n", in);
            }
        }
        // Command: move player (w/a/s/d for direction)
        else if (strchr("wasd", buf[0])) {
            // Loop through each character in the input to process multiple moves in one command
            for (size_t i = 0; buf[i]; i++) {
                char c = buf[i];
                if (c=='w' || c=='a' || c=='s' || c=='d') {
                    // Try to move the player in the specified direction
                    int res = move_player(world, c);
                    // Movement failed (e.g. hit a wall)
                    if (res == 0) {
                        printf("Cannot move '%c'.\n", c);
                    // Picked up an item (Health Potion)
                    } else if (res == 2) {
                        // Picked up an item (Health Potion)
                        printf("You found: Health Potion\n");
                        printf("Item added to inventory!\n\n");
                        printf("=== INVENTORY ===\n");
                        for (int j = 0; j < world->inventory_count; j++) {
                            printf("%d. Health Potion\n", j+1);
                        }
                        printf("Total items: %d\n\n", world->inventory_count);
                    }
                }
            }
            // Redraw the world map after movemen
            draw_world(world);
        }
        // Command: look around (show tile types around player)
        else if (strcmp(buf, "l") == 0) {
            ManagedEntity *pp = entity_get(&world->em, world->player_id);
            printf("Looking around...\n");
            for (int dy=-1; dy<=1; dy++) {
                for (int dx=-1; dx<=1; dx++) {
                    if (dx==0 && dy==0) continue;
                    int nx = pp->x + dx, ny = pp->y + dy;
                    if (nx<0 || nx>=world->width || ny<0 || ny>=world->height) continue;
                    TileType t = world->tiles[ny][nx].type;
                    printf("(%d,%d): %s %s\n", nx, ny,
                           t==TILE_WALL?"Wall":"Empty",
                           world->tiles[ny][nx].occupant?"(Entity)":"");
                }
            }
        }
        // Command: show inventory and HP
        else if (strcmp(buf, "i") == 0) {
            printf("Inventory:\n");
            printf("HP: %d/%d\n", world->player_hp, PLAYER_MAX_HP);
            for (int j=0; j<world->inventory_count; j++) {
                printf("%d. Health Potion\n", j+1);
            }
            printf("Total items: %d\n", world->inventory_count);
        }
        // Command: use health potion (if any)
        else if (strcmp(buf, "u") == 0) {
            if (world->inventory_count > 0) {
                world->player_hp = PLAYER_MAX_HP;
                world->inventory_count--;
                printf("You used a Health Potion! HP is now %d/%d\n\n",
                    world->player_hp, PLAYER_MAX_HP);
            } else {
                printf("You have no potions to use.\n\n");
            }
        }
        // Unknown command
        else {
            printf("Unknown command: %s\n", buf);
        }
    }

    // Clean up and exit
    world_free(world);
    printf("Exiting game.\n");
    return 0;
}
