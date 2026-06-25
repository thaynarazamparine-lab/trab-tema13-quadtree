#include "raylib.h"
#include "../include/quadtree.h"
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NUM_PARTICLES 60

void draw_quadtree_bounds(QuadTreeNode* node) {
    if (!node) return;
    DrawRectangleLines((int)node->bounds.x, (int)node->bounds.y, 
                       (int)node->bounds.width, (int)node->bounds.height, DARKGRAY);
    if (node->is_divided) {
        draw_quadtree_bounds(node->nw);
        draw_quadtree_bounds(node->ne);
        draw_quadtree_bounds(node->sw);
        draw_quadtree_bounds(node->se);
    }
}

int main() {
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simulador de Colisoes - Quadtree");
    SetTargetFPS(60);

    Particle particles[NUM_PARTICLES];
    double vx[NUM_PARTICLES];
    double vy[NUM_PARTICLES];

    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].id = i;
        particles[i].position.x = (double)(rand() % (SCREEN_WIDTH - 40) + 20);
        particles[i].position.y = (double)(rand() % (SCREEN_HEIGHT - 40) + 20);
        particles[i].radius = 4.0;
        vx[i] = (double)((rand() % 200 - 100) / 50.0);
        vy[i] = (double)((rand() % 200 - 100) / 50.0);
    }

    while (!WindowShouldClose()) {
        for (int i = 0; i < NUM_PARTICLES; i++) {
            particles[i].position.x += vx[i];
            particles[i].position.y += vy[i];

            if (particles[i].position.x - particles[i].radius <= 0 || 
                particles[i].position.x + particles[i].radius >= SCREEN_WIDTH) vx[i] *= -1;
            if (particles[i].position.y - particles[i].radius <= 0 || 
                particles[i].position.y + particles[i].radius >= SCREEN_HEIGHT) vy[i] *= -1;
        }

        AABB boundary = {0.0, 0.0, (double)SCREEN_WIDTH, (double)SCREEN_HEIGHT};
        QuadTreeNode* root = create_node(boundary);
        
        for (int i = 0; i < NUM_PARTICLES; i++) insert_particle(root, &particles[i]);

        bool is_colliding[NUM_PARTICLES] = {false};
        for (int i = 0; i < NUM_PARTICLES; i++) {
            Particle* collided[10];
            int collision_count = 0;
            check_collisions(root, &particles[i], collided, &collision_count);
            if (collision_count > 0) is_colliding[i] = true;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        draw_quadtree_bounds(root);

        for (int i = 0; i < NUM_PARTICLES; i++) {
            Color c = is_colliding[i] ? RED : MAROON;
            DrawCircle((int)particles[i].position.x, (int)particles[i].position.y, 
                       (float)particles[i].radius, c);
        }

        DrawFPS(10, 10);
        EndDrawing();
        free_tree(root);
    }
    CloseWindow();
    return 0;
}
