#include "../include/quadtree.h"
#include <stdlib.h>
#include <stdio.h>

/* =========================================================
 * Auxiliares geométricos
 * ========================================================= */

/* Retorna true se (x,y) está dentro de box (borda direita/superior inclusive) */
bool aabb_contains(AABB box, double x, double y) {
    return x >= box.x && x <= box.x + box.width &&
           y >= box.y && y <= box.y + box.height;
}

/* Retorna true se os dois AABBs se intersectam */
bool aabb_intersects(AABB a, AABB b) {
    return !(b.x > a.x + a.width  ||
             b.x + b.width  < a.x ||
             b.y > a.y + a.height ||
             b.y + b.height < a.y);
}

/* =========================================================
 * Criação e destruição
 * ========================================================= */

QuadTreeNode *create_node(AABB bounds) {
    QuadTreeNode *node = (QuadTreeNode *)malloc(sizeof(QuadTreeNode));
    if (!node) {
        fprintf(stderr, "create_node: malloc falhou\n");
        exit(EXIT_FAILURE);
    }
    node->bounds     = bounds;
    node->count      = 0;
    node->is_divided = false;
    node->nw = node->ne = node->sw = node->se = NULL;
    for (int i = 0; i < MAX_PARTICLES; i++)
        node->particles[i] = NULL;
    return node;
}

/* Destrói recursivamente todos os filhos e o próprio nó */
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

/* Zera o conteúdo do nó e destrói os filhos sem liberar o nó raiz.
 * Útil para reutilizar o nó raiz entre frames. */
void clear_tree(QuadTreeNode *node) {
    if (!node) return;
    if (node->is_divided) {
        free_tree(node->nw);
        free_tree(node->ne);
        free_tree(node->sw);
        free_tree(node->se);
        node->nw = node->ne = node->sw = node->se = NULL;
    }
    node->count      = 0;
    node->is_divided = false;
    for (int i = 0; i < MAX_PARTICLES; i++)
        node->particles[i] = NULL;
}

/* =========================================================
 * Subdivisão
 * ========================================================= */

/* Cria os quatro filhos e redistribui as partículas do nó pai.
 * Chamado apenas quando o nó ainda não está dividido. */
void subdivide(QuadTreeNode *node) {
    double x = node->bounds.x;
    double y = node->bounds.y;
    double w = node->bounds.width  / 2.0;
    double h = node->bounds.height / 2.0;

    /* Convenção de coordenadas: y cresce para baixo (Raylib).
     *   NW = esquerda/cima   NE = direita/cima
     *   SW = esquerda/baixo  SE = direita/baixo */
    node->nw = create_node((AABB){x,     y,     w, h});
    node->ne = create_node((AABB){x + w, y,     w, h});
    node->sw = create_node((AABB){x,     y + h, w, h});
    node->se = create_node((AABB){x + w, y + h, w, h});
    node->is_divided = true;

    /* Redistribui as partículas existentes para os filhos corretos */
    for (int i = 0; i < node->count; i++) {
        Particle *p = node->particles[i];
        if (!insert_particle(node->nw, p) &&
            !insert_particle(node->ne, p) &&
            !insert_particle(node->sw, p) &&
            !insert_particle(node->se, p)) {
            /* Não deveria acontecer — partícula estava dentro dos limites */
            fprintf(stderr, "subdivide: nao foi possivel redistribuir particula %d\n", p->id);
        }
        node->particles[i] = NULL;
    }
    node->count = 0;
}

/* =========================================================
 * Fusão (merge)
 * ========================================================= */

/* Conta o total de partículas em toda a subárvore */
static int subtree_count(QuadTreeNode *node) {
    if (!node) return 0;
    if (!node->is_divided) return node->count;
    return subtree_count(node->nw) + subtree_count(node->ne) +
           subtree_count(node->sw) + subtree_count(node->se);
}

/* Coleta todas as partículas da subárvore em 'out', retorna quantas coletou */
static int collect_particles(QuadTreeNode *node, Particle **out, int max) {
    if (!node) return 0;
    int n = 0;
    if (!node->is_divided) {
        for (int i = 0; i < node->count && n < max; i++)
            out[n++] = node->particles[i];
        return n;
    }
    n += collect_particles(node->nw, out + n, max - n);
    n += collect_particles(node->ne, out + n, max - n);
    n += collect_particles(node->sw, out + n, max - n);
    n += collect_particles(node->se, out + n, max - n);
    return n;
}

/* Retorna true se o nó pai deve reabsorver os filhos */
bool should_merge(QuadTreeNode *node) {
    if (!node || !node->is_divided) return false;
    return subtree_count(node) <= MERGE_THRESHOLD;
}

/* Reabsorve os filhos no nó pai liberando sua memória */
void merge(QuadTreeNode *node) {
    if (!node || !node->is_divided) return;

    Particle *buf[MAX_PARTICLES];
    int n = collect_particles(node, buf, MAX_PARTICLES);

    free_tree(node->nw);
    free_tree(node->ne);
    free_tree(node->sw);
    free_tree(node->se);
    node->nw = node->ne = node->sw = node->se = NULL;
    node->is_divided = false;

    node->count = 0;
    for (int i = 0; i < n; i++) {
        node->particles[i] = buf[i];
        node->count++;
    }
}

/* =========================================================
 * Inserção
 * ========================================================= */

bool insert_particle(QuadTreeNode *node, Particle *p) {
    if (!node || !p) return false;

    if (!aabb_contains(node->bounds, p->position.x, p->position.y))
        return false;

    /* Nó folha com espaço disponível */
    if (!node->is_divided && node->count < MAX_PARTICLES) {
        node->particles[node->count++] = p;
        return true;
    }

    /* Precisa subdividir */
    if (!node->is_divided)
        subdivide(node);

    /* Encaminha para o filho correto */
    if (insert_particle(node->nw, p)) return true;
    if (insert_particle(node->ne, p)) return true;
    if (insert_particle(node->sw, p)) return true;
    if (insert_particle(node->se, p)) return true;

    return false;
}

/* =========================================================
 * Remoção
 * ========================================================= */

bool remove_particle(QuadTreeNode *node, Particle *p) {
    if (!node || !p) return false;

    if (!aabb_contains(node->bounds, p->position.x, p->position.y))
        return false;

    if (!node->is_divided) {
        /* Procura linearmente no vetor do nó folha */
        for (int i = 0; i < node->count; i++) {
            if (node->particles[i] == p) {
                /* Preenche o buraco com o último elemento */
                node->particles[i] = node->particles[node->count - 1];
                node->particles[node->count - 1] = NULL;
                node->count--;
                return true;
            }
        }
        return false;
    }

    /* Nó interno: desce para o filho que pode conter a partícula */
    bool removed = remove_particle(node->nw, p) ||
                   remove_particle(node->ne, p) ||
                   remove_particle(node->sw, p) ||
                   remove_particle(node->se, p);

    /* Após remoção, verifica se vale fundir os filhos */
    if (removed && should_merge(node))
        merge(node);

    return removed;
}

/* =========================================================
 * Consulta espacial (range query)
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
