#include "../include/quadtree.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

/* =========================================================
 * Criação e destruição
 * ========================================================= */

QuadTreeNode *create_node(AABB bounds) {
    QuadTreeNode *node = (QuadTreeNode *)malloc(sizeof(QuadTreeNode));
    if (!node) return NULL;
    node->bounds     = bounds;
    node->count      = 0;
    node->is_divided = false;
    node->nw = node->ne = node->sw = node->se = NULL;
    return node;
}

void free_tree(QuadTreeNode *node) {
    if (!node) return;
    if (node->is_divided) {
        free_tree(node->nw);
        free_tree(node->ne);
        free_tree(node->sw);
        free_tree(node->se);
    }
    free(node);
}

void clear_tree(QuadTreeNode *node) {
    if (!node) return;
    if (node->is_divided) {
        free_tree(node->nw);
        free_tree(node->ne);
        free_tree(node->sw);
        free_tree(node->se);
        node->nw = node->ne = node->sw = node->se = NULL;
        node->is_divided = false;
    }
    node->count = 0;
}

/* =========================================================
 * Subdivisão e fusão
 * ========================================================= */

void subdivide(QuadTreeNode *node) {
    double x = node->bounds.x;
    double y = node->bounds.y;
    double w = node->bounds.width  / 2.0;
    double h = node->bounds.height / 2.0;

    node->nw = create_node((AABB){x,     y + h, w, h});
    node->ne = create_node((AABB){x + w, y + h, w, h});
    node->sw = create_node((AABB){x,     y,     w, h});
    node->se = create_node((AABB){x + w, y,     w, h});
    node->is_divided = true;

    /* Redistribui partículas existentes nos filhos */
    for (int i = 0; i < node->count; i++) {
        Particle *p = node->particles[i];
        if (!insert_particle(node->nw, p) &&
            !insert_particle(node->ne, p) &&
            !insert_particle(node->sw, p) &&
            !insert_particle(node->se, p)) {
            /* Não deveria acontecer — partícula estava dentro dos bounds */
        }
    }
    node->count = 0;
}

bool should_merge(QuadTreeNode *node) {
    if (!node || !node->is_divided) return false;
    if (node->nw->is_divided || node->ne->is_divided ||
        node->sw->is_divided || node->se->is_divided)
        return false;
    int total = node->nw->count + node->ne->count +
                node->sw->count + node->se->count;
    return total <= MERGE_THRESHOLD;
}

void merge(QuadTreeNode *node) {
    if (!node || !node->is_divided) return;
    QuadTreeNode *children[4] = { node->nw, node->ne, node->sw, node->se };
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i < children[c]->count; i++) {
            if (node->count < MAX_PARTICLES)
                node->particles[node->count++] = children[c]->particles[i];
        }
    }
    for (int c = 0; c < 4; c++) { free(children[c]); }
    node->nw = node->ne = node->sw = node->se = NULL;
    node->is_divided = false;
}

/* =========================================================
 * Inserção e remoção
 * ========================================================= */

bool insert_particle(QuadTreeNode *node, Particle *p) {
    if (!node) return false;

    if (p->position.x < node->bounds.x ||
        p->position.x > node->bounds.x + node->bounds.width  ||
        p->position.y < node->bounds.y ||
        p->position.y > node->bounds.y + node->bounds.height)
        return false;

    if (!node->is_divided && node->count < MAX_PARTICLES) {
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

bool remove_particle(QuadTreeNode *node, Particle *p) {
    if (!node) return false;

    if (p->position.x < node->bounds.x ||
        p->position.x > node->bounds.x + node->bounds.width  ||
        p->position.y < node->bounds.y ||
        p->position.y > node->bounds.y + node->bounds.height)
        return false;

    for (int i = 0; i < node->count; i++) {
        if (node->particles[i] == p) {
            node->particles[i] = node->particles[--node->count];
            return true;
        }
    }

    if (node->is_divided) {
        bool removed = remove_particle(node->nw, p) ||
                       remove_particle(node->ne, p) ||
                       remove_particle(node->sw, p) ||
                       remove_particle(node->se, p);
        if (removed && should_merge(node)) merge(node);
        return removed;
    }
    return false;
}

/* =========================================================
 * Auxiliares geométricos
 * ========================================================= */

bool aabb_contains(AABB box, double x, double y) {
    return x >= box.x && x <= box.x + box.width &&
           y >= box.y && y <= box.y + box.height;
}

bool aabb_intersects(AABB a, AABB b) {
    return !(b.x > a.x + a.width  ||
             b.x + b.width  < a.x ||
             b.y > a.y + a.height ||
             b.y + b.height < a.y);
}

/* Distância ao quadrado do ponto (px,py) ao retângulo 'box'.
 * Retorna 0 se o ponto está dentro — peça central da poda das buscas. */
double dist2_point_aabb(AABB box, double px, double py) {
    double dx = 0.0, dy = 0.0;
    if (px < box.x)                   dx = box.x - px;
    else if (px > box.x + box.width)  dx = px - (box.x + box.width);
    if (py < box.y)                   dy = box.y - py;
    else if (py > box.y + box.height) dy = py - (box.y + box.height);
    return dx * dx + dy * dy;
}

/* =========================================================
 * Consulta por retângulo
 * ========================================================= */

int query_range(QuadTreeNode *node, AABB range,
                Particle **results, int max_results) {
    if (!node || max_results <= 0) return 0;
    if (!aabb_intersects(node->bounds, range)) return 0;

    int found = 0;
    if (!node->is_divided) {
        for (int i = 0; i < node->count && found < max_results; i++) {
            Particle *p = node->particles[i];
            if (aabb_contains(range, p->position.x, p->position.y))
                results[found++] = p;
        }
        return found;
    }
    found += query_range(node->nw, range, results + found, max_results - found);
    found += query_range(node->ne, range, results + found, max_results - found);
    found += query_range(node->sw, range, results + found, max_results - found);
    found += query_range(node->se, range, results + found, max_results - found);
    return found;
}

/* =========================================================
 * Consulta por região (interface da Thaynara — mantida)
 * ========================================================= */

void query_region(QuadTreeNode *node, AABB range,
                  Particle **found, int *found_count) {
    if (!node || !aabb_intersects(node->bounds, range)) return;
    for (int i = 0; i < node->count; i++) {
        Particle *p = node->particles[i];
        if (aabb_contains(range, p->position.x, p->position.y))
            found[(*found_count)++] = p;
    }
    if (node->is_divided) {
        query_region(node->nw, range, found, found_count);
        query_region(node->ne, range, found, found_count);
        query_region(node->sw, range, found, found_count);
        query_region(node->se, range, found, found_count);
    }
}

/* =========================================================
 * Detecção de colisão (Thaynara — mantida)
 * ========================================================= */

static double get_distance(Particle *p1, Particle *p2) {
    double dx = p1->position.x - p2->position.x;
    double dy = p1->position.y - p2->position.y;
    return sqrt(dx * dx + dy * dy);
}

void check_collisions(QuadTreeNode *root, Particle *p,
                      Particle **collided, int *collision_count) {
    AABB range = {
        p->position.x - (p->radius * 2),
        p->position.y - (p->radius * 2),
        p->radius * 4,
        p->radius * 4
    };
    Particle *found[100];
    int found_count = 0;
    query_region(root, range, found, &found_count);
    for (int i = 0; i < found_count; i++) {
        if (found[i] != p &&
            get_distance(p, found[i]) <= (p->radius + found[i]->radius))
            collided[(*collision_count)++] = found[i];
    }
}

/* =========================================================
 * Busca por raio (Rafael Zoppé)
 * ========================================================= */

int query_radius(QuadTreeNode *node, double cx, double cy, double radius,
                 Particle **results, int max_results) {
    if (!node || max_results <= 0) return 0;
    double r2 = radius * radius;
    if (dist2_point_aabb(node->bounds, cx, cy) > r2) return 0;

    int found = 0;
    if (!node->is_divided) {
        for (int i = 0; i < node->count && found < max_results; i++) {
            Particle *p = node->particles[i];
            double dx = p->position.x - cx;
            double dy = p->position.y - cy;
            if (dx * dx + dy * dy <= r2)
                results[found++] = p;
        }
        return found;
    }
    found += query_radius(node->nw, cx, cy, radius, results + found, max_results - found);
    found += query_radius(node->ne, cx, cy, radius, results + found, max_results - found);
    found += query_radius(node->sw, cx, cy, radius, results + found, max_results - found);
    found += query_radius(node->se, cx, cy, radius, results + found, max_results - found);
    return found;
}

/* =========================================================
 * Vizinho mais próximo — branch-and-bound (Rafael Zoppé)
 * ========================================================= */

static void nearest_recursive(QuadTreeNode *node, double px, double py,
                               Particle **best, double *best_d2) {
    if (!node) return;
    if (*best != NULL && dist2_point_aabb(node->bounds, px, py) >= *best_d2)
        return;

    if (!node->is_divided) {
        for (int i = 0; i < node->count; i++) {
            Particle *p = node->particles[i];
            double dx = p->position.x - px;
            double dy = p->position.y - py;
            double d2 = dx * dx + dy * dy;
            if (*best == NULL || d2 < *best_d2) {
                *best_d2 = d2;
                *best    = p;
            }
        }
        return;
    }

    QuadTreeNode *child[4] = { node->nw, node->ne, node->sw, node->se };
    double        cd[4];
    for (int i = 0; i < 4; i++)
        cd[i] = child[i] ? dist2_point_aabb(child[i]->bounds, px, py) : DBL_MAX;

    /* Insertion sort nos 4 filhos por distância (visita o mais próximo primeiro) */
    for (int i = 1; i < 4; i++) {
        QuadTreeNode *cn = child[i]; double dd = cd[i]; int j = i - 1;
        while (j >= 0 && cd[j] > dd) { child[j+1]=child[j]; cd[j+1]=cd[j]; j--; }
        child[j+1] = cn; cd[j+1] = dd;
    }
    for (int i = 0; i < 4; i++)
        nearest_recursive(child[i], px, py, best, best_d2);
}

Particle *find_nearest(QuadTreeNode *node, double px, double py) {
    Particle *best    = NULL;
    double    best_d2 = 0.0;
    nearest_recursive(node, px, py, &best, &best_d2);
    return best;
}
