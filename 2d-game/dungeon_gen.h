#ifndef DUNGEON_GEN_H
#define DUNGEON_GEN_H

#include "world.h"  // Tile, TileType, World

// Room information structure
typedef struct {
    int x, y, w, h;  // Top-left coordinates (x, y) and size (width, height)
    int cx, cy;      // Center coordinates (for corridor connections)
} Room;

// Checks if two rooms overlap
// Returns: nonzero if overlapping, 0 otherwise
int room_overlap(const Room *a, const Room *b);

// Generates a dungeon map
//   tiles  - 2D array of Tile objects (typically world->tiles)
//   width  - Width of the map
//   height - Height of the map
void dungeon_gen(Tile **tiles, int width, int height);

#endif 
