#include "dungeon_gen.h"
#include <stdlib.h>
#include <time.h>

#define MAX_ROOMS 30

// Checks if two rooms overlap.
int room_overlap(const Room *a, const Room *b) {
    if (a->x < b->x + b->w &&
        a->x + a->w > b->x &&
        a->y < b->y + b->h &&
        a->y + a->h > b->y) {
        return 1;  // overlap
    } else {
        return 0;  // no overlap
    }
}

void dungeon_gen(Tile **tiles, int width, int height) {
    // Set all tiles to wall
    srand((unsigned)time(NULL));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            tiles[y][x].type = TILE_WALL;
            tiles[y][x].occupant = NULL;
        }
    }

    // Generate rooms
    Room rooms[MAX_ROOMS];
    int room_count = 0;

    // Create the first room at (1,1), guaranteed to include the start position
    int rw = rand() % 7 + 4;  // width: 4–10
    int rh = rand() % 3 + 4;  // height: 4–6
    Room r0 = {1, 1, rw, rh, 1 + rw/2, 1 + rh/2};
    // Carve out the room as empty floor
    for (int yy = r0.y; yy < r0.y + r0.h; yy++) {
        for (int xx = r0.x; xx < r0.x + r0.w; xx++) {
            tiles[yy][xx].type = TILE_EMPTY;
        }
    }
    rooms[room_count++] = r0;

    // Try to place additional rooms at random positions
    for (int i = 1; i < MAX_ROOMS; i++) {
        int attempts = 0;
        Room r;
        int placed = 0;
        // Try up to MAX_ROOMS times to find a non-overlapping placement for this room
        while (attempts < MAX_ROOMS) {
            // Randomly choose room size and position
            r.w = rand() % 7 + 4;
            r.h = rand() % 3 + 4;
            r.x = rand() % (width - r.w - 2) + 1;
            r.y = rand() % (height - r.h - 2) + 1;
            r.cx = r.x + r.w / 2;
            r.cy = r.y + r.h / 2;
            attempts++;

            // Check overlap with all previously placed rooms
            int ok = 1;
            for (int j = 0; j < room_count; j++) {
                if (room_overlap(&r, &rooms[j])) {
                    ok = 0;
                    break;
                }
            }
            if (ok) {
                placed = 1;
                break;
            }
        }
        // Carve out the room as empty floor
        if (placed) {
            for (int yy = r.y; yy < r.y + r.h; yy++) {
                for (int xx = r.x; xx < r.x + r.w; xx++) {
                    tiles[yy][xx].type = TILE_EMPTY;
                }
            }
            rooms[room_count++] = r;
        }
    }

    // Sort rooms by center x-coordinate to connect them in order from left to right.
    // This ensures all rooms are connected by a single path through the dungeon.
    for (int i = 0; i < room_count - 1; i++) {
        for (int j = i + 1; j < room_count; j++) {
            if (rooms[i].cx > rooms[j].cx) {
                Room tmp = rooms[i];
                rooms[i] = rooms[j];
                rooms[j] = tmp;
            }
        }
    }

    // Connect adjacent rooms with L-shaped corridors
    for (int i = 0; i < room_count - 1; i++) {
        Room *a = &rooms[i];
        Room *b = &rooms[i+1];
        int x = a->cx, y = a->cy;
        // First, move horizontally from a to b, then vertically
        while (x != b->cx) {
            tiles[y][x].type = TILE_EMPTY;
            x += (b->cx > x) ? 1 : -1;
        }
        while (y != b->cy) {
            tiles[y][x].type = TILE_EMPTY;
            y += (b->cy > y) ? 1 : -1;
        }
    }
}
