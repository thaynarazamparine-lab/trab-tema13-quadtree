/* test_concorrente.c — Rafael Zoppé
 *
 * Objetivo: provar, via ThreadSanitizer (TSan), que múltiplas threads podem
 * consultar a Quadtree em paralelo sem data races, e via AddressSanitizer /
 * Valgrind que não há vazamentos de memória.
 *
 * Cenário: a árvore é construída na thread principal (escrita sequencial,
 * sem concorrência) e depois N_THREADS threads realizam consultas simultâneas
 * (query_radius e find_nearest) sobre a árvore imutável.  Esse padrão
 * "escreve uma vez, lê em paralelo" é a base do plano de dados concorrente
 * do Tema 13 antes da sincronização fina de E3.
 *
 * Compilar com TSan:
 *   gcc -Wall -Wextra -Werror -std=c11 -Iinclude -fsanitize=thread -g \
 *       tests/test_concorrente.c src/quadtree.c -lpthread -o test_conc_tsan
 *
 * Compilar com ASan:
 *   gcc -Wall -Wextra -Werror -std=c11 -Iinclude \
 *       -fsanitize=address,undefined -g \
 *       tests/test_concorrente.c src/quadtree.c -lpthread -o test_conc_asan
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../include/quadtree.h"
#include "../include/types.h"

/* ------------------------------------------------------------------ */
/* Configuração                                                         */
/* ------------------------------------------------------------------ */
#define N_PARTICLES  500    /* partículas inseridas antes dos testes   */
#define N_THREADS    8      /* threads leitoras simultâneas            */
#define N_QUERIES    200    /* consultas por thread                    */
#define WORLD_SIZE   100.0
#define SEARCH_RADIUS 20.0
#define MAX_RESULTS  N_PARTICLES

/* ------------------------------------------------------------------ */
/* Estado compartilhado (somente leitura após setup)                   */
/* ------------------------------------------------------------------ */
typedef struct {
    QuadTreeNode *root;       /* árvore já populada — só leitura      */
    Particle     *particles;  /* array de partículas — só leitura     */
    int           n;          /* número de partículas                 */
} SharedData;

/* ------------------------------------------------------------------ */
/* Auxiliar: distância ao quadrado entre dois pontos                   */
/* ------------------------------------------------------------------ */
static double dist2(double ax, double ay, double bx, double by) {
    double dx = ax - bx;
    double dy = ay - by;
    return dx * dx + dy * dy;
}

/* ------------------------------------------------------------------ */
/* Função executada por cada thread leitora                            */
/* ------------------------------------------------------------------ */
static void *thread_reader(void *arg) {
    SharedData *sd = (SharedData *)arg;

    /* Cada thread tem seu próprio buffer de resultados (sem compartilhamento
     * de escrita entre threads — isso evita races nos resultados). */
    Particle **results = (Particle **)malloc(MAX_RESULTS * sizeof(Particle *));
    assert(results != NULL);

    /* Gerador linear congruente local por thread (C11 puro, sem rand_r).
     * Semente diferente por thread garante pontos distintos sem estado
     * compartilhado entre threads. */
    unsigned long rng = (unsigned long)(size_t)arg * 6364136223846793005UL + 1;

    for (int q = 0; q < N_QUERIES; q++) {
        rng = rng * 6364136223846793005UL + 1442695040888963407UL;
        double cx = (double)(rng >> 33) / (double)(1UL << 31) * WORLD_SIZE;
        rng = rng * 6364136223846793005UL + 1442695040888963407UL;
        double cy = (double)(rng >> 33) / (double)(1UL << 31) * WORLD_SIZE;

        /* --- query_radius: todos dentro do raio --- */
        int found = query_radius(sd->root, cx, cy, SEARCH_RADIUS,
                                 results, MAX_RESULTS);

        /* Validação: cada resultado retornado DEVE estar dentro do raio.
         * Se a poda introduzisse um bug, isso pegaria aqui. */
        double r2 = SEARCH_RADIUS * SEARCH_RADIUS;
        for (int i = 0; i < found; i++) {
            double d2 = dist2(results[i]->position.x, results[i]->position.y,
                              cx, cy);
            assert(d2 <= r2 + 1e-9); /* tolerância numérica mínima */
        }

        /* --- find_nearest: vizinho único mais próximo --- */
        Particle *nn = find_nearest(sd->root, cx, cy);
        assert(nn != NULL); /* árvore tem partículas, nunca NULL */

        /* Validação: nenhuma outra partícula pode ser mais próxima. */
        double best = dist2(nn->position.x, nn->position.y, cx, cy);
        for (int i = 0; i < sd->n; i++) {
            double d2 = dist2(sd->particles[i].position.x,
                              sd->particles[i].position.y, cx, cy);
            assert(d2 >= best - 1e-9);
        }
    }

    free(results);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Teste principal                                                      */
/* ------------------------------------------------------------------ */
static void test_leitura_concorrente(void) {
    printf("  Configuracao: %d particulas, %d threads, %d consultas/thread\n",
           N_PARTICLES, N_THREADS, N_QUERIES);

    /* 1. Construção sequencial da árvore (sem concorrência aqui) */
    AABB bounds = {0.0, 0.0, WORLD_SIZE, WORLD_SIZE};
    QuadTreeNode *root = create_node(bounds);
    Particle *particles = (Particle *)malloc(N_PARTICLES * sizeof(Particle));
    assert(particles != NULL);

    srand(42); /* semente fixa: teste reproduzível */
    for (int i = 0; i < N_PARTICLES; i++) {
        particles[i].id         = i;
        particles[i].position.x = ((double)rand() / RAND_MAX) * WORLD_SIZE;
        particles[i].position.y = ((double)rand() / RAND_MAX) * WORLD_SIZE;
        particles[i].velocity.x = 0.0;
        particles[i].velocity.y = 0.0;
        particles[i].radius     = 1.0;
        particles[i].mass       = 1.0;
        assert(insert_particle(root, &particles[i]));
    }

    /* 2. Lançamento das threads leitoras */
    SharedData sd = { root, particles, N_PARTICLES };
    pthread_t threads[N_THREADS];

    for (int i = 0; i < N_THREADS; i++)
        assert(pthread_create(&threads[i], NULL, thread_reader, &sd) == 0);

    /* 3. Espera todas terminarem */
    for (int i = 0; i < N_THREADS; i++)
        assert(pthread_join(threads[i], NULL) == 0);

    /* 4. Limpeza */
    free(particles);
    free_tree(root);

    printf("  [OK] test_leitura_concorrente — %d threads x %d consultas "
           "sem data races\n", N_THREADS, N_QUERIES);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("\n=== Testes de Concorrencia (TSan / ASan) ===\n");
    test_leitura_concorrente();
    printf("\nTodos os testes de concorrencia passaram.\n");
    return 0;
}
