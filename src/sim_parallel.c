/* Requer _XOPEN_SOURCE >= 600 para pthread_barrier_t e CLOCK_MONOTONIC */
#define _XOPEN_SOURCE 600

/*
 * sim_parallel.c — Simulação de partículas multi-thread com Quadtree
 *
 * Arquitetura:
 *   - O espaço 800x600 é dividido em N_THREADS faixas horizontais.
 *   - Cada thread possui sua Quadtree local, coberta pela sua faixa.
 *   - Quando uma partícula ultrapassa a fronteira, a thread a coloca
 *     na TransferQueue da thread de destino.
 *   - No início de cada frame a thread drena sua própria fila de entrada
 *     e insere as partículas recebidas na Quadtree local.
 *   - A thread principal coleta as métricas e as grava em CSV.
 *
 * Sincronização:
 *   - Barreira (pthread_barrier) separa as fases:
 *       1. update + detecção de migração
 *       2. drenagem das filas + rebalanceamento (reconstrução)
 *       3. coleta de métricas
 *   - Cada TransferQueue tem seu próprio mutex — sem risco de deadlock.
 */

#include "../include/quadtree.h"
#include "../include/transfer.h"
#include "../include/metrics.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* =========================================================
 * Parâmetros da simulação
 * ========================================================= */
#define WORLD_W      800.0
#define WORLD_H      600.0
#define N_THREADS    4
#define TOTAL_FRAMES 300

/* Cenário normal */
#define N_PARTICLES_NORMAL  200

/* Cenário de desbalanceamento: 80% em 10% da área (canto superior esquerdo) */
#define N_PARTICLES_IMBAL   500
#define IMBAL_HOT_FRAC      0.80  /* fração das partículas na região quente */
#define IMBAL_HOT_W         80.0  /* largura da região quente */
#define IMBAL_HOT_H         60.0  /* altura da região quente */
#define IMBAL_SEED          42

/* Diretórios de saída */
#define CSV_DIR_NORMAL "results/csv/normal"
#define CSV_DIR_IMBAL  "results/csv/imbalance"

/* =========================================================
 * Estrutura por thread
 * ========================================================= */
typedef struct {
    int            id;
    int            num_threads;
    AABB           region;        /* faixa horizontal desta thread */
    Particle      *all_particles; /* array global de partículas */
    int            n_particles;   /* total de partículas no array global */
    double        *vx;            /* velocidades x (array global) */
    double        *vy;            /* velocidades y (array global) */

    /* Quadtree local desta thread */
    QuadTreeNode  *tree;

    /* Filas de entrada de transferência (uma por thread de origem) */
    TransferQueue *in_queues;   /* in_queues[num_threads] — array externo */

    /* Pointer para as filas de entrada de cada outra thread (para enqueue) */
    TransferQueue **peer_queues; /* peer_queues[i] aponta para in_queues[i] da thread i */

    /* Barreira compartilhada */
    pthread_barrier_t *barrier;

    /* Métricas do frame atual (preenchidas pela thread) */
    ThreadMetrics  metrics;

    /* Flag de controle */
    int  total_frames;
    int *current_frame; /* ponteiro para o frame global (leitura apenas) */
} ThreadArg;

/* =========================================================
 * Determina a qual thread pertence a posição y
 * ========================================================= */
static int thread_for_y(double y, int num_threads) {
    double stripe_h = WORLD_H / (double)num_threads;
    int t = (int)(y / stripe_h);
    if (t < 0) t = 0;
    if (t >= num_threads) t = num_threads - 1;
    return t;
}

/* =========================================================
 * Função executada por cada thread
 * ========================================================= */
static void *thread_func(void *arg) {
    ThreadArg *a = (ThreadArg *)arg;

    for (int frame = 0; frame < a->total_frames; frame++) {

        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        /* ---- Zera métricas do frame ---- */
        memset(&a->metrics, 0, sizeof(a->metrics));
        a->metrics.thread_id = a->id;

        /* ---- FASE 1: move partículas da minha faixa, detecta migrações ---- */
        for (int i = 0; i < a->n_particles; i++) {
            Particle *p = &a->all_particles[i];

            /* Só processa partículas que estão na minha faixa ANTES do movimento */
            if (!aabb_contains(a->region, p->position.x, p->position.y))
                continue;

            /* Move */
            p->position.x += a->vx[i];
            p->position.y += a->vy[i];

            /* Rebate nas bordas do mundo */
            if (p->position.x - p->radius <= 0.0 ||
                p->position.x + p->radius >= WORLD_W)
                a->vx[i] *= -1.0;
            if (p->position.y - p->radius <= 0.0 ||
                p->position.y + p->radius >= WORLD_H)
                a->vy[i] *= -1.0;

            /* Corrige para não sair do mundo */
            if (p->position.x < 0.0)      p->position.x = 0.0;
            if (p->position.x > WORLD_W)  p->position.x = WORLD_W;
            if (p->position.y < 0.0)      p->position.y = 0.0;
            if (p->position.y > WORLD_H)  p->position.y = WORLD_H;

            /* Detecta migração para outra thread */
            int dest = thread_for_y(p->position.y, a->num_threads);
            if (dest != a->id) {
                transfer_enqueue(a->peer_queues[dest], p);
                a->metrics.migrations_sent++;
            }
        }

        /* ---- Barreira 1: todos terminaram movimento e enfileiram migrações ---- */
        pthread_barrier_wait(a->barrier);

        /* ---- FASE 2: drena filas de entrada + reconstrói Quadtree local ---- */
        free_tree(a->tree);
        a->tree = create_node(a->region);

        /* Absorve partículas recebidas */
        Particle *incoming[TRANSFER_QUEUE_CAP];
        int rcv = transfer_drain(&a->in_queues[a->id], incoming, TRANSFER_QUEUE_CAP);
        a->metrics.migrations_received = rcv;

        /* Insere partículas da minha faixa (posição atual) */
        int inserted = 0;
        for (int i = 0; i < a->n_particles; i++) {
            Particle *p = &a->all_particles[i];
            if (aabb_contains(a->region, p->position.x, p->position.y)) {
                insert_particle(a->tree, p);
                inserted++;
            }
        }
        /* Insere partículas recebidas que ficaram na minha faixa */
        for (int i = 0; i < rcv; i++) {
            if (aabb_contains(a->region, incoming[i]->position.x, incoming[i]->position.y)) {
                insert_particle(a->tree, incoming[i]);
                inserted++;
            }
        }
        a->metrics.particles_processed = inserted;
        a->metrics.insertions          = inserted;

        /* ---- Barreira 2: todas as Quadtrees locais estão reconstruídas ---- */
        pthread_barrier_wait(a->barrier);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        a->metrics.time_spent_ms =
            (double)(t1.tv_sec - t0.tv_sec) * 1000.0 +
            (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;

        /* ---- Barreira 3: métricas prontas — thread principal coleta ---- */
        pthread_barrier_wait(a->barrier);
    }

    return NULL;
}

/* =========================================================
 * Inicializa partículas para o cenário normal (aleatório)
 * ========================================================= */
static void init_normal(Particle *ps, double *vx, double *vy, int n, unsigned seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        ps[i].id         = i;
        ps[i].position.x = (double)(rand() % (int)(WORLD_W - 10) + 5);
        ps[i].position.y = (double)(rand() % (int)(WORLD_H - 10) + 5);
        ps[i].radius     = 3.0;
        ps[i].mass       = 1.0;
        vx[i] = ((rand() % 200) - 100) / 60.0;
        vy[i] = ((rand() % 200) - 100) / 60.0;
    }
}

/* =========================================================
 * Inicializa partículas para o cenário de desbalanceamento
 * 80% na região quente (canto NW), 20% no restante
 * ========================================================= */
static void init_imbalanced(Particle *ps, double *vx, double *vy, int n, unsigned seed) {
    srand(seed);
    int hot = (int)(n * IMBAL_HOT_FRAC);

    for (int i = 0; i < n; i++) {
        ps[i].id     = i;
        ps[i].radius = 2.0;
        ps[i].mass   = 1.0;

        if (i < hot) {
            /* Região quente: x in [0, IMBAL_HOT_W], y in [0, IMBAL_HOT_H] */
            ps[i].position.x = (double)(rand() % (int)IMBAL_HOT_W);
            ps[i].position.y = (double)(rand() % (int)IMBAL_HOT_H);
        } else {
            /* Resto da área — evita a região quente */
            double x, y;
            do {
                x = (double)(rand() % (int)(WORLD_W - 4) + 2);
                y = (double)(rand() % (int)(WORLD_H - 4) + 2);
            } while (x < IMBAL_HOT_W && y < IMBAL_HOT_H);
            ps[i].position.x = x;
            ps[i].position.y = y;
        }
        vx[i] = ((rand() % 100) - 50) / 80.0;  /* velocidades menores: partículas permanecem concentradas */
        vy[i] = ((rand() % 100) - 50) / 80.0;
    }
}

/* =========================================================
 * Simulação com coleta frame a frame (versão headless completa)
 * ========================================================= */
static void run_full(Particle *particles, double *vx, double *vy,
                     int n, const char *csv_dir) {

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s results/graphs results/logs", csv_dir);
    if (system(cmd) == -1) {}

    CsvHandles csv = metrics_open_csv(csv_dir);

    TransferQueue in_queues[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        transfer_queue_init(&in_queues[i]);

    TransferQueue *peer_queues[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        peer_queues[i] = &in_queues[i];

    /* Barreira para N_THREADS + thread principal */
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, N_THREADS + 1);

    double stripe_h = WORLD_H / (double)N_THREADS;

    ThreadArg args[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        args[i].id            = i;
        args[i].num_threads   = N_THREADS;
        args[i].region        = (AABB){0.0, i * stripe_h, WORLD_W, stripe_h};
        args[i].all_particles = particles;
        args[i].n_particles   = n;
        args[i].vx            = vx;
        args[i].vy            = vy;
        args[i].tree          = create_node(args[i].region);
        args[i].in_queues     = in_queues;
        args[i].peer_queues   = peer_queues;
        args[i].barrier       = &barrier;
        args[i].total_frames  = TOTAL_FRAMES;
    }

    pthread_t tids[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        pthread_create(&tids[i], NULL, thread_func, &args[i]);

    /* Thread principal participa das barreiras para coletar métricas por frame */
    for (int frame = 0; frame < TOTAL_FRAMES; frame++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);

        /* Aguarda barreira 1 (movimento concluído) */
        pthread_barrier_wait(&barrier);
        /* Aguarda barreira 2 (reconstrução concluída) */
        pthread_barrier_wait(&barrier);
        /* Aguarda barreira 3 (métricas das threads prontas) */
        pthread_barrier_wait(&barrier);

        clock_gettime(CLOCK_MONOTONIC, &t1);

        FrameMetrics fm;
        memset(&fm, 0, sizeof(fm));
        fm.frame       = frame;
        fm.num_threads = N_THREADS;
        fm.time_ms     = (double)(t1.tv_sec - t0.tv_sec) * 1000.0 +
                         (double)(t1.tv_nsec - t0.tv_nsec) / 1e6;

        int total_mig = 0;
        for (int i = 0; i < N_THREADS; i++) {
            fm.threads[i] = args[i].metrics;
            total_mig    += args[i].metrics.migrations_sent;
        }
        fm.migrations = total_mig;
        fm.balance    = metrics_calc_balance(fm.threads, N_THREADS);
        fm.tree       = metrics_collect_tree(args[0].tree);

        metrics_write_frame(&csv, &fm);
    }

    for (int i = 0; i < N_THREADS; i++)
        pthread_join(tids[i], NULL);

    for (int i = 0; i < N_THREADS; i++) {
        free_tree(args[i].tree);
        transfer_queue_destroy(&in_queues[i]);
    }
    pthread_barrier_destroy(&barrier);
    metrics_close_csv(&csv);
}

/* =========================================================
 * main
 * ========================================================= */
int main(int argc, char *argv[]) {
    int imbalanced = 0;
    if (argc > 1 && argv[1][0] == 'i') imbalanced = 1;

    if (imbalanced) {
        printf("Cenario: DESBALANCEAMENTO (seed=%d, %d particulas, "
               "%.0f%% em %.0fx%.0f)\n",
               IMBAL_SEED, N_PARTICLES_IMBAL,
               IMBAL_HOT_FRAC * 100.0, IMBAL_HOT_W, IMBAL_HOT_H);

        Particle *ps = malloc(N_PARTICLES_IMBAL * sizeof(Particle));
        double   *vx = malloc(N_PARTICLES_IMBAL * sizeof(double));
        double   *vy = malloc(N_PARTICLES_IMBAL * sizeof(double));

        init_imbalanced(ps, vx, vy, N_PARTICLES_IMBAL, IMBAL_SEED);
        run_full(ps, vx, vy, N_PARTICLES_IMBAL, CSV_DIR_IMBAL);

        free(ps); free(vx); free(vy);
        printf("CSVs em: %s\n", CSV_DIR_IMBAL);
    } else {
        printf("Cenario: NORMAL (%d particulas, %d threads, %d frames)\n",
               N_PARTICLES_NORMAL, N_THREADS, TOTAL_FRAMES);

        Particle *ps = malloc(N_PARTICLES_NORMAL * sizeof(Particle));
        double   *vx = malloc(N_PARTICLES_NORMAL * sizeof(double));
        double   *vy = malloc(N_PARTICLES_NORMAL * sizeof(double));

        init_normal(ps, vx, vy, N_PARTICLES_NORMAL, 0);
        run_full(ps, vx, vy, N_PARTICLES_NORMAL, CSV_DIR_NORMAL);

        free(ps); free(vx); free(vy);
        printf("CSVs em: %s\n", CSV_DIR_NORMAL);
    }

    printf("Execute: python3 scripts/plot_metrics.py\n");
    return 0;
}
