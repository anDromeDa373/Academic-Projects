#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#define PLAYER_MAX_HP 3
#include "db_engine.h"  

// Define EntityType here
typedef enum {
    ENTITY_PLAYER,   // Player entity
    ENTITY_ENEMY,    // Enemy entity
    ENTITY_ITEM,     // Item entity
    // Add more types if needed
} EntityType;

typedef struct {
    int id;      // Unique ID for the entity
    int type;    // Type of entity (see EntityType)
    int x;       // X position on the map
    int y;       // Y position on the map
    int hp;      // Hit points (health points)
    int active;  // 1 if entity is active, 0 if not
} ManagedEntity;

typedef struct {
    MemoryPool     pool;          // Memory pool for efficient allocation
    ManagedEntity *entities;      // Array of managed entities
    int            next_id;       // ID to assign to the next created entity
    size_t         max_entities;  // Maximum number of entities
} EntityManager;

// Initialize the EntityManager
// em: pointer to EntityManager
// max_entities: maximum number of entities
void entity_manager_init(EntityManager *em, size_t max_entities);

// Create a new entity and add it to the manager
// em: pointer to EntityManager
// type: entity type (see EntityType)
// x: initial X position
// y: initial Y position
// Returns: the ID of the new entity, or 0 if creation failed
int entity_create(EntityManager *em, int type, int x, int y);

// Get a pointer to the entity with the given ID
// em: pointer to EntityManager
// id: entity ID
// Returns: pointer to the ManagedEntity, or NULL if not found
ManagedEntity* entity_get(EntityManager *em, int id);

// Delete entity by ID, return 1 if deleted, 0 if not found
int entity_delete(EntityManager *em, int id);

// Update entity position, return 1 if success, 0 if not found
int entity_update(EntityManager *em, int id, int x, int y);

// Return the number of entities
int entity_count(const EntityManager *em);

// Destroy the EntityManager and free resources
void entity_manager_destroy(EntityManager *em);

#endif
