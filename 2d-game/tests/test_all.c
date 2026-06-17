#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "world.h"

#define TEST_FILE "tmp_save.dat"

/* ---------------------------------------------------------------------------
   Helper: Rough equality check between two World instances                     
   ---------------------------------------------------------------------------*/
static int world_equal(World *a, World *b)
{
    if (!a || !b) return 0;
    if (a->width != b->width || a->height != b->height) return 0;
    /* Compare tile types */
    for (int y = 0; y < a->height; y++)
        for (int x = 0; x < a->width; x++)
            if (a->tiles[y][x].type != b->tiles[y][x].type)
                return 0;
    /* Compare active-entity counts */
    if (entity_count(&a->em) != entity_count(&b->em)) return 0;
    return 1;                               /* good-enough equality */
}

/* ---------------------------------------------------------------------------
   1. Happy-path CRUD test: create → get → update → delete                     
   ---------------------------------------------------------------------------*/
void test_entity_crud(void)
{
    puts("[TEST] entity CRUD");
    EntityManager em; entity_manager_init(&em, 4);

    int id = entity_create(&em, ENTITY_PLAYER, 1, 1);
    assert(id == 1);

    ManagedEntity *e = entity_get(&em, id);
    assert(e && e->x == 1 && e->y == 1);

    assert(entity_update(&em, id, 3, 4));     /* move entity */
    e = entity_get(&em, id);
    assert(e->x == 3 && e->y == 4);

    assert(entity_delete(&em, id));            /* logical delete */
    assert(entity_count(&em) == 0);

    entity_manager_destroy(&em);
    puts("  OK");
}

/* ---------------------------------------------------------------------------
   2. Negative test: invalid ID should return NULL / 0                         
   ---------------------------------------------------------------------------*/
void test_invalid_id(void)
{
    puts("[TEST] invalid ID");
    EntityManager em; entity_manager_init(&em, 2);

    assert(entity_get(&em, 99) == NULL);
    assert(entity_delete(&em, 99) == 0);

    entity_manager_destroy(&em);
    puts("  OK");
}

/* ---------------------------------------------------------------------------
   3. Boundary & HP test: wall collision + enemy hit                           
   ---------------------------------------------------------------------------*/
void test_boundary_and_hp(void)
{
    puts("[TEST] boundary + HP");
    World *w = world_create(20, 20);

    /* Move into a wall: expect failure */
    assert(move_player(w, 'w') == 0);

    /* Place an enemy to the right of the player and expect HP-1 */
    ManagedEntity *p = entity_get(&w->em, w->player_id);
    int ex = p->x + 1, ey = p->y;

    int eid = entity_create(&w->em, ENTITY_ENEMY, ex, ey);
    w->tiles[ey][ex].occupant = entity_get(&w->em, eid);

    int hp_before = w->player_hp;
    assert(move_player(w, 'd') == 1);          /* collide and move */
    assert(w->player_hp == hp_before - 1);

    world_free(w);
    puts("  OK");
}

/* ---------------------------------------------------------------------------
   4. Persistence test: save → load should give identical state               
   ---------------------------------------------------------------------------*/
void test_save_load_equal(void)
{
    puts("[TEST] save/load equality");
    World *w1 = world_create(20, 20);

    /* Make a small change so the world isn't pristine */
    move_player(w1, 'd');

    assert(save_world(w1, TEST_FILE));

    World *w2 = load_world(TEST_FILE);
    assert(world_equal(w1, w2));

    world_free(w1);
    world_free(w2);
    remove(TEST_FILE);
    puts("  OK");
}

/* ---------------------------------------------------------------------------
   Entry point: run all tests                                                  
   ---------------------------------------------------------------------------*/
int main(void)
{
    test_entity_crud();
    test_invalid_id();
    test_boundary_and_hp();
    test_save_load_equal();

    puts("All tests passed!");
    return 0;
}