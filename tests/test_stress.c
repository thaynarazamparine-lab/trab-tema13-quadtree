#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/quadtree.h"

#define STRESS_N    1000
#define WORLD_SIZE  800.0

static double randf(double lo, double hi) {
    return lo + (hi - lo) * ((double)rand() / RAND_MAX);
}

/* Insere N partículas, consulta toda a área e remove todas */
static void stress_insert_query_remove(void) {
    AABB bounds = {0.0, 0.0, WORLD_SIZE, WORLD_SIZE};
    QuadTreeNode *root = create_node(bounds);

    Particle *particles = (Particle *)malloc(STRESS_N * sizeof(Particle));
    assert(particles != NULL);

    for (int i = 0; i < STRESS_N; i++) {
        particles[i].id         = i;
        particles[i].position.x = randf(0.0, WORLD_SIZE);
        particles[i].position.y = randf(0.0, WORLD_SIZE);
        particles[i].velocity.x = 0.0;
        particles[i].velocity.y = 0.0;
        particles[i].radius     = 1.0;
        particles[i].mass       = 1.0;
        assert(insert_particle(root, &particles[i]) == true);
    }

    /* Query de toda a área deve retornar todas as partículas */
    Particle **results = (Particle **)malloc(STRESS_N * sizeof(Particle *));
    assert(results != NULL);
    int found = query_range(root, bounds, results, STRESS_N);
    assert(found == STRESS_N);
    free(results);

    /* Remove todas */
    for (int i = 0; i < STRESS_N; i++)
        remove_particle(root, &particles[i]);

    free(particles);
    free_tree(root);
    printf("  [OK] stress_insert_query_remove (%d particulas)\n", STRESS_N);
}

/* Reconstrói a árvore 100 vezes (simula frames) */
static void stress_rebuild_frames(void) {
    AABB bounds = {0.0, 0.0, WORLD_SIZE, WORLD_SIZE};

    Particle particles[100];
    double vx[100], vy[100];
    for (int i = 0; i < 100; i++) {
        particles[i].id         = i;
        particles[i].position.x = randf(0.0, WORLD_SIZE);
        particles[i].position.y = randf(0.0, WORLD_SIZE);
        particles[i].radius     = 4.0;
        vx[i] = randf(-2.0, 2.0);
        vy[i] = randf(-2.0, 2.0);
    }

    for (int frame = 0; frame < 100; frame++) {
        for (int i = 0; i < 100; i++) {
            particles[i].position.x += vx[i];
            particles[i].position.y += vy[i];
            if (particles[i].position.x < 0.0 || particles[i].position.x > WORLD_SIZE) vx[i] *= -1;
            if (particles[i].position.y < 0.0 || particles[i].position.y > WORLD_SIZE) vy[i] *= -1;
        }
        QuadTreeNode *root = create_node(bounds);
        for (int i = 0; i < 100; i++)
            insert_particle(root, &particles[i]);
        free_tree(root);
    }
    printf("  [OK] stress_rebuild_frames (100 frames, 100 particulas)\n");
}

int main(void) {
    srand((unsigned)time(NULL));
    printf("\n=== Testes de Stress ===\n");
    stress_insert_query_remove();
    stress_rebuild_frames();
    printf("\nTodos os testes de stress passaram.\n");
    return 0;
}
