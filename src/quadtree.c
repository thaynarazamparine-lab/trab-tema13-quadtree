#include "../include/quadtree.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

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

/* Distância ao quadrado do ponto (px,py) ao retângulo 'box'.
 * Para cada eixo, mede o quanto o ponto está "fora" da faixa do retângulo;
 * se está dentro da faixa, a contribuição daquele eixo é zero. Logo:
 *   - ponto dentro do retângulo  -> retorna 0
 *   - ponto fora                 -> retorna o quadrado da distância até a
 *                                   borda/canto mais próximo
 * Sem sqrt: comparamos sempre distâncias ao quadrado, o que é exato e barato. */
double dist2_point_aabb(AABB box, double px, double py) {
    double dx = 0.0;
    double dy = 0.0;
    if (px < box.x)                   dx = box.x - px;
    else if (px > box.x + box.width)  dx = px - (box.x + box.width);
    if (py < box.y)                   dy = box.y - py;
    else if (py > box.y + box.height) dy = py - (box.y + box.height);
    return dx * dx + dy * dy;
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

/* =========================================================
 * Consulta por raio (vizinhos dentro de um círculo)
 * ========================================================= */

int query_radius(QuadTreeNode *node, double cx, double cy, double radius,
                 Particle **results, int max_results) {
    if (!node || max_results <= 0) return 0;

    double r2 = radius * radius;

    /* PODA: se o ponto mais próximo do retângulo deste nó já está mais longe
     * que o raio, nenhuma partícula da subárvore inteira pode entrar no
     * círculo. Descartamos o nó e todos os seus descendentes de uma vez. */
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
 * Vizinho mais próximo (branch-and-bound)
 * ========================================================= */

/* Percorre a árvore mantendo o melhor candidato encontrado até agora
 * (*best, com distância ao quadrado *best_d2). A poda usa dist2_point_aabb:
 * um nó cujo retângulo está mais longe que o melhor atual não pode conter
 * nada melhor, então é ignorado. Visitar primeiro o filho mais próximo faz
 * o "melhor" ficar pequeno cedo, o que poda mais os filhos seguintes. */
static void nearest_recursive(QuadTreeNode *node, double px, double py,
                              Particle **best, double *best_d2) {
    if (!node) return;

    /* Se já temos um candidato e este nó não pode superá-lo, poda. */
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

    /* Ordena os quatro filhos pela distância do retângulo ao ponto, do mais
     * perto ao mais longe (insertion sort em 4 elementos). */
    QuadTreeNode *child[4] = { node->nw, node->ne, node->sw, node->se };
    double        cd[4];
    for (int i = 0; i < 4; i++)
        cd[i] = child[i] ? dist2_point_aabb(child[i]->bounds, px, py) : DBL_MAX;

    for (int i = 1; i < 4; i++) {
        QuadTreeNode *cn = child[i];
        double        dd = cd[i];
        int j = i - 1;
        while (j >= 0 && cd[j] > dd) {
            child[j + 1] = child[j];
            cd[j + 1]    = cd[j];
            j--;
        }
        child[j + 1] = cn;
        cd[j + 1]    = dd;
    }

    for (int i = 0; i < 4; i++)
        nearest_recursive(child[i], px, py, best, best_d2);
}

Particle *find_nearest(QuadTreeNode *node, double px, double py) {
    Particle *best    = NULL;
    double    best_d2 = 0.0;   /* só é lido depois que best != NULL */
    nearest_recursive(node, px, py, &best, &best_d2);
    return best;
}
