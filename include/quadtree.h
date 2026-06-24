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

/* --- Auxiliares --- */
bool aabb_contains(AABB box, double x, double y);
bool aabb_intersects(AABB a, AABB b);

#endif /* QUADTREE_H */
