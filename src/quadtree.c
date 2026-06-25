#include "../include/quadtree.h"
#include <stdlib.h>
#include <math.h>

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

    if (!node->is_divided) subdivide(node);

    if (insert_particle(node->nw, p)) return true;
    if (insert_particle(node->ne, p)) return true;
    if (insert_particle(node->sw, p)) return true;
    if (insert_particle(node->se, p)) return true;

    return false;
}

bool remove_particle(QuadTreeNode* node, Particle* p) {
    if (!node) return false;

    if (p->position.x < node->bounds.x || p->position.x > node->bounds.x + node->bounds.width ||
        p->position.y < node->bounds.y || p->position.y > node->bounds.y + node->bounds.height) {
        return false;
    }

    for (int i = 0; i < node->count; i++) {
        if (node->particles[i] == p) {
            node->particles[i] = node->particles[--node->count];
            return true;
        }
    }

    if (node->is_divided) {
        if (remove_particle(node->nw, p)) return true;
        if (remove_particle(node->ne, p)) return true;
        if (remove_particle(node->sw, p)) return true;
        if (remove_particle(node->se, p)) return true;
    }
    return false;
}

bool intersects(AABB a, AABB b) {
    return !(a.x > b.x + b.width || a.x + a.width < b.x || a.y > b.y + b.height || a.y + a.height < b.y);
}

void query_region(QuadTreeNode* node, AABB range, Particle** found, int* found_count) {
    if (!node || !intersects(node->bounds, range)) return;

    for (int i = 0; i < node->count; i++) {
        Particle* p = node->particles[i];
        if (p->position.x >= range.x && p->position.x <= range.x + range.width &&
            p->position.y >= range.y && p->position.y <= range.y + range.height) {
            found[(*found_count)++] = p;
        }
    }

    if (node->is_divided) {
        query_region(node->nw, range, found, found_count);
        query_region(node->ne, range, found, found_count);
        query_region(node->sw, range, found, found_count);
        query_region(node->se, range, found, found_count);
    }
}

double get_distance(Particle* p1, Particle* p2) {
    double dx = p1->position.x - p2->position.x;
    double dy = p1->position.y - p2->position.y;
    return sqrt(dx * dx + dy * dy);
}

void check_collisions(QuadTreeNode* root, Particle* p, Particle** collided, int* collision_count) {
    AABB range = {
        p->position.x - (p->radius * 2),
        p->position.y - (p->radius * 2),
        p->radius * 4,
        p->radius * 4
    };

    Particle* found[100];
    int found_count = 0;
    query_region(root, range, found, &found_count);

    for (int i = 0; i < found_count; i++) {
        if (found[i] != p && get_distance(p, found[i]) <= (p->radius + found[i]->radius)) {
            collided[(*collision_count)++] = found[i];
        }
    }
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
