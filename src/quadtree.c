#include "../include/quadtree.h"
#include <stdlib.h>

QuadTreeNode* create_node(AABB bounds) {
    QuadTreeNode* node = (QuadTreeNode*)malloc(sizeof(QuadTreeNode));
    node->bounds = bounds;
    node->count = 0;
    node->is_divided = false;
    node->nw = node->ne = node->sw = node->se = NULL;
    return node;
}

void subdivide(QuadTreeNode* node) {
    double x = node->bounds.x;
    double y = node->bounds.y;
    double w = node->bounds.width / 2.0;
    double h = node->bounds.height / 2.0;

    node->nw = create_node((AABB){x, y + h, w, h});
    node->ne = create_node((AABB){x + w, y + h, w, h});
    node->sw = create_node((AABB){x, y, w, h});
    node->se = create_node((AABB){x + w, y, w, h});
    node->is_divided = true;
}

bool insert_particle(QuadTreeNode* node, Particle* p) {
    if (!node) return false;
    
    if (p->position.x < node->bounds.x || p->position.x > node->bounds.x + node->bounds.width ||
        p->position.y < node->bounds.y || p->position.y > node->bounds.y + node->bounds.height) {
        return false;
    }

    if (node->count < MAX_PARTICLES && !node->is_divided) {
        node->particles[node->count++] = p;
        return true;
    }

    if (!node->is_divided) {
        subdivide(node);
    }

    if (insert_particle(node->nw, p)) return true;
    if (insert_particle(node->ne, p)) return true;
    if (insert_particle(node->sw, p)) return true;
    if (insert_particle(node->se, p)) return true;

    return false;
}

void free_tree(QuadTreeNode* node) {
    if (!node) return;
    if (node->is_divided) {
        free_tree(node->nw);
        free_tree(node->ne);
        free_tree(node->sw);
        free_tree(node->se);
    }
    free(node);
}
