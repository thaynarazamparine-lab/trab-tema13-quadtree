#ifndef QUADTREE_H
#define QUADTREE_H

#include "types.h"
#include <stdbool.h>

#define MAX_PARTICLES 4

typedef struct QuadTreeNode {
    AABB bounds;
    Particle* particles[MAX_PARTICLES];
    int count;
    bool is_divided;
    struct QuadTreeNode *nw, *ne, *sw, *se;
} QuadTreeNode;

QuadTreeNode* create_node(AABB bounds);
bool insert_particle(QuadTreeNode* node, Particle* p);
void free_tree(QuadTreeNode* node);

#endif
