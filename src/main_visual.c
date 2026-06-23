#include "raylib.h"
#include "../include/quadtree.h"
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NUM_PARTICLES 60

void draw_quadtree_bounds(QuadTreeNode* node) {
    if (!node) return;
    
    // Desenha as linhas de grade da árvore em cinza escuro
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

    // Inicializa as partículas em posições aleatórias
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].id = i;
        particles[i].position.x = (double)(rand() % (SCREEN_WIDTH - 40) + 20);
        particles[i].position.y = (double)(rand() % (SCREEN_HEIGHT - 40) + 20);
        particles[i].radius = 4.0;
        
        // Velocidades aleatórias
        vx[i] = (double)((rand() % 200 - 100) / 50.0);
        vy[i] = (double)((rand() % 200 - 100) / 50.0);
    }

    while (!WindowShouldClose()) {
        // 1. Atualiza as posições das partículas e rebate nas bordas da tela
        for (int i = 0; i < NUM_PARTICLES; i++) {
            particles[i].position.x += vx[i];
            particles[i].position.y += vy[i];

            if (particles[i].position.x - particles[i].radius <= 0 || 
                particles[i].position.x + particles[i].radius >= SCREEN_WIDTH) {
                vx[i] *= -1;
            }
            if (particles[i].position.y - particles[i].radius <= 0 || 
                particles[i].position.y + particles[i].radius >= SCREEN_HEIGHT) {
                vy[i] *= -1;
            }
        }

        // 2. Reconstrói a Quadtree a cada frame com o espaço total da janela
        AABB boundary = {0.0, 0.0, (double)SCREEN_WIDTH, (double)SCREEN_HEIGHT};
        QuadTreeNode* root = create_node(boundary);
        
        for (int i = 0; i < NUM_PARTICLES; i++) {
            insert_particle(root, &particles[i]);
        }

        // 3. Renderização
        BeginDrawing();
        ClearBackground(BLACK);

        // Desenha as divisões espaciais da árvore
        draw_quadtree_bounds(root);

        // Desenha as partículas na tela
        for (int i = 0; i < NUM_PARTICLES; i++) {
            DrawCircle((int)particles[i].position.x, (int)particles[i].position.y, 
                       (float)particles[i].radius, MAROON);
        }

        DrawFPS(10, 10);
        EndDrawing();

        free_tree(root);
    }

    CloseWindow();
    return 0;
}
