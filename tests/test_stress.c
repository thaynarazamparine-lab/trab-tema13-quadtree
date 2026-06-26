#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/quadtree.h"

void check_collisions_brute(Particle* p, Particle* particles, int n, Particle** collided, int* count) {
    for (int i = 0; i < n; i++) {
        if (p->id != particles[i].id) {
            double dx = p->position.x - particles[i].position.x;
            double dy = p->position.y - particles[i].position.y;
            if ((dx * dx + dy * dy) <= (p->radius + particles[i].radius) * (p->radius + particles[i].radius)) {
                collided[(*count)++] = &particles[i];
            }
        }
    }
}

void run_benchmark(int n) {
    Particle* particles = malloc(n * sizeof(Particle));
    for (int i = 0; i < n; i++) {
        particles[i].id = i;
        particles[i].position.x = rand() % 800;
        particles[i].position.y = rand() % 600;
        particles[i].radius = 4.0;
    }

    clock_t start, end;
    double time_brute = 0, time_quad = 0;
    Particle* collided[100];
    int count = 0;

    start = clock();
    for (int i = 0; i < n; i++) {
        count = 0;
        check_collisions_brute(&particles[i], particles, n, collided, &count);
    }
    end = clock();
    time_brute = ((double)(end - start)) / CLOCKS_PER_SEC;

    start = clock();
    AABB boundary = {0, 0, 800, 600};
    QuadTreeNode* root = create_node(boundary);
    for (int i = 0; i < n; i++) insert_particle(root, &particles[i]);
    for (int i = 0; i < n; i++) {
        count = 0;
        check_collisions(root, &particles[i], collided, &count);
    }
    free_tree(root);
    end = clock();
    time_quad = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("%d,%.6f,%.6f\n", n, time_brute, time_quad);
    free(particles);
}

int main() {
    printf("N,Tempo_O(n2),Tempo_Quadtree\n");
    int test_sizes[] = {100, 500, 1000, 2500, 5000, 10000};
    for (int i = 0; i < 6; i++) {
        run_benchmark(test_sizes[i]);
    }
    return 0;
}
