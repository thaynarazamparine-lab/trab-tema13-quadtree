#include "raylib.h"
#include "../include/quadtree.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define NUM_PARTICLES 60

void DrawHeart(int x, int y, float size, Color color) {
    float r = size * 0.5f;
    DrawCircle(x - (int)r, y - (int)r, r, color);
    DrawCircle(x + (int)r, y - (int)r, r, color);
    DrawTriangle((Vector2){x - size, y - r + 1.0f}, 
                 (Vector2){x, y + size}, 
                 (Vector2){x + size, y - r + 1.0f}, color);
}

void draw_quadtree_bounds(QuadTreeNode* node, bool* is_colliding) {
    if (!node) return;
    
    bool has_collision = false;
    for (int i = 0; i < node->count; i++) {
        if (is_colliding[node->particles[i]->id]) {
            has_collision = true;
            break;
        }
    }

    if (has_collision) {
        DrawRectangle((int)node->bounds.x, (int)node->bounds.y, 
                      (int)node->bounds.width, (int)node->bounds.height, Fade(WHITE, 0.05f));
        DrawRectangleLines((int)node->bounds.x, (int)node->bounds.y, 
                           (int)node->bounds.width, (int)node->bounds.height, Fade(WHITE, 0.3f));
    } else {
        DrawRectangleLines((int)node->bounds.x, (int)node->bounds.y, 
                           (int)node->bounds.width, (int)node->bounds.height, DARKGRAY);
    }

    if (node->is_divided) {
        draw_quadtree_bounds(node->nw, is_colliding);
        draw_quadtree_bounds(node->ne, is_colliding);
        draw_quadtree_bounds(node->sw, is_colliding);
        draw_quadtree_bounds(node->se, is_colliding);
    }
}

int main() {
    srand(time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Simulador de Colisoes - Quadtree");
    SetTargetFPS(60);

    Particle particles[NUM_PARTICLES];
    double vx[NUM_PARTICLES];
    double vy[NUM_PARTICLES];

    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].id = i;
        particles[i].position.x = (double)(rand() % (800 - 40) + 20);
        particles[i].position.y = (double)(rand() % (600 - 40) + 20);
        particles[i].radius = 6.0; 
        vx[i] = (double)((rand() % 200 - 100) / 30.0);
        vy[i] = (double)((rand() % 200 - 100) / 30.0);
    }

    while (!WindowShouldClose()) {
        int current_width = GetScreenWidth();
        int current_height = GetScreenHeight();

        for (int i = 0; i < NUM_PARTICLES; i++) {
            particles[i].position.x += vx[i];
            particles[i].position.y += vy[i];

            if (particles[i].position.x - particles[i].radius <= 0) {
                particles[i].position.x = particles[i].radius;
                vx[i] = fabs(vx[i]);
            } else if (particles[i].position.x + particles[i].radius >= current_width) {
                particles[i].position.x = current_width - particles[i].radius;
                vx[i] = -fabs(vx[i]);
            }

            if (particles[i].position.y - particles[i].radius <= 0) {
                particles[i].position.y = particles[i].radius;
                vy[i] = fabs(vy[i]);
            } else if (particles[i].position.y + particles[i].radius >= current_height) {
                particles[i].position.y = current_height - particles[i].radius;
                vy[i] = -fabs(vy[i]);
            }
        }

        AABB boundary = {0.0, 0.0, (double)current_width, (double)current_height};
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
        
        draw_quadtree_bounds(root, is_colliding);

        for (int i = 0; i < NUM_PARTICLES; i++) {
            Color c = is_colliding[i] ? PINK : BLUE;
            DrawHeart((int)particles[i].position.x, (int)particles[i].position.y, 
                      (float)particles[i].radius, c);
        }

        DrawFPS(10, 10);
        EndDrawing();
        free_tree(root);
    }
    CloseWindow();
    return 0;
}