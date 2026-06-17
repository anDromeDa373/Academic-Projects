#include "entity_manager.h"
#include <stdlib.h>
#define ENEMY_MAX_HP 3

// Allocate max_entities ManagedEntity objects using a memory pool
// Initialize EntityManager and its memory pool.
// All entities are set inactive (not in use) at start.
void entity_manager_init(EntityManager *em, size_t max_entities) {
    pool_init(&em->pool, max_entities, sizeof(ManagedEntity));
    em->entities     = (ManagedEntity*)em->pool.memory;
    em->max_entities = max_entities;
    em->next_id      = 1;
    // Mark all as inactive for easy slot reuse.
    for (size_t i = 0; i < max_entities; i++) {
        em->entities[i].active = 0;
        em->entities[i].hp     = 0;
    }
}

// Create a new entity and add it to the pool.
// Returns the new entity's ID, or -1 if no slot is available.
int entity_create(EntityManager *em, int type, int x, int y) {
    ManagedEntity *e = (ManagedEntity*)pool_alloc(&em->pool);
    if (!e) {
        return -1; // Pool is full
    }

    e->id     = em->next_id++;
    e->type   = type;
    e->x      = x;
    e->y      = y;
    e->active = 1;

    // Set HP depending on entity type
    if (type == ENTITY_PLAYER) {
        e->hp = PLAYER_MAX_HP;
    }      
    else if (type == ENTITY_ENEMY) {
        e->hp = 0; // Enemy disappears when hit.
    } 
    else {
        e->hp = 0; // Items etc.
    }                      
    return e->id;
}

// Find active entity by ID. Returns pointer, or NULL if not found.
ManagedEntity* entity_get(EntityManager *em, int id) {
    if (id <= 0) {
        return NULL;
    }
    for (size_t i = 0; i < em->max_entities; i++) {
        ManagedEntity *e = &em->entities[i];
        if (e->active && e->id == id) {
            return e;
        }
    }
    return NULL;
}

// Mark entity as inactive and free its slot. Return 1 if deleted, 0 if not found.
int entity_delete(EntityManager *em, int id) {
    ManagedEntity *e = entity_get(em, id);
    if (!e) {
        return 0;
    }
    e->active = 0;
    pool_free(&em->pool, e);
    return 1;
}

// Update entity position. Return 1 if success, 0 if not found.
int entity_update(EntityManager *em, int id, int x, int y) {
    ManagedEntity *e = entity_get(em, id);
    if (!e) {
        return 0;
    }
    e->x = x;
    e->y = y;
    return 1;
}

// Count active entities.
int entity_count(const EntityManager *em) {
    int cnt = 0;
    for (size_t i = 0; i < em->max_entities; i++) {
        if (em->entities[i].active) {
            cnt++;
        }
    }
    return cnt;
}

// Free all resources used by the EntityManager.
void entity_manager_destroy(EntityManager *em) {
    pool_destroy(&em->pool);
    em->entities     = NULL;
    em->max_entities = 0;
    em->next_id      = 0;
}
