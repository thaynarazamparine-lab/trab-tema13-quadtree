#ifndef QUADTREE_H
#define QUADTREE_H

#include "types.h"
#include <stdbool.h>

/* Capacidade máxima de partículas por nó antes de subdividir */
#define MAX_PARTICLES 4

/* Threshold para fusão: se o total de partículas nos 4 filhos
 * for <= MERGE_THRESHOLD, o nó pai reabsorve os filhos */
#define MERGE_THRESHOLD MAX_PARTICLES

typedef struct QuadTreeNode {
    AABB bounds;
    Particle *particles[MAX_PARTICLES];
    int count;
    bool is_divided;
    struct QuadTreeNode *nw, *ne, *sw, *se;
} QuadTreeNode;

/* --- Criação e destruição --- */
QuadTreeNode *create_node(AABB bounds);
void          free_tree(QuadTreeNode *node);
void          clear_tree(QuadTreeNode *node);   /* zera sem liberar o nó raiz */

/* --- Inserção --- */
bool insert_particle(QuadTreeNode *node, Particle *p);

/* --- Remoção --- */
bool remove_particle(QuadTreeNode *node, Particle *p);

/* --- Subdivisão e fusão --- */
void subdivide(QuadTreeNode *node);
bool should_merge(QuadTreeNode *node);
void merge(QuadTreeNode *node);

/* --- Consulta espacial --- */

/* Preenche 'results' com ponteiros para partículas cujos centros estão
 * dentro de 'range'. Retorna a quantidade encontrada. */
int query_range(QuadTreeNode *node, AABB range,
                Particle **results, int max_results);

/* --- Consulta de vizinhos (base da detecção de colisão) --- */

/* Preenche 'results' com as partículas cujo centro está a uma distância
 * <= 'radius' de (cx, cy). Retorna quantas encontrou (no máx. max_results).
 * Poda subárvores cujo retângulo está todo fora do círculo de busca. */
int query_radius(QuadTreeNode *node, double cx, double cy, double radius,
                 Particle **results, int max_results);

/* Retorna a partícula mais próxima de (px, py), ou NULL se a árvore estiver
 * vazia. Usa poda branch-and-bound: visita primeiro o quadrante mais próximo
 * e descarta os que não podem conter nada mais perto que o melhor já achado. */
Particle *find_nearest(QuadTreeNode *node, double px, double py);

/* --- Auxiliares --- */
bool   aabb_contains(AABB box, double x, double y);
bool   aabb_intersects(AABB a, AABB b);

/* Distância AO QUADRADO do ponto (px, py) ao retângulo 'box'.
 * Retorna 0 se o ponto está dentro. Trabalhar com o quadrado evita sqrt
 * e mantém a comparação exata — é a peça central da poda das duas buscas. */
double dist2_point_aabb(AABB box, double px, double py);

#endif /* QUADTREE_H */
