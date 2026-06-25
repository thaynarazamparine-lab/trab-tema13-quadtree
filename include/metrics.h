#ifndef METRICS_H
#define METRICS_H

#include <stdio.h>
#include <math.h>

/* Número máximo de threads suportadas */
#define MAX_THREADS 8

/* =========================================================
 * Métricas por thread (por frame)
 * ========================================================= */
typedef struct {
    int    thread_id;
    int    particles_processed;  /* partículas na Quadtree local */
    double time_spent_ms;        /* tempo de update desta thread */
    int    migrations_sent;      /* partículas enviadas para outra thread */
    int    migrations_received;  /* partículas recebidas de outras threads */
    int    insertions;
    int    removals;
    int    subdivisions;
    int    merges;
} ThreadMetrics;

/* =========================================================
 * Métricas da Quadtree (por frame)
 * ========================================================= */
typedef struct {
    int    total_nodes;
    int    max_depth;
    double avg_depth;
} TreeMetrics;

/* =========================================================
 * Métricas de balanceamento (por frame, calculadas sobre threads)
 * ========================================================= */
typedef struct {
    double max_load;
    double min_load;
    double avg_load;
    double stddev_load;
    double imbalance;  /* (max - avg) / avg */
} BalanceMetrics;

/* =========================================================
 * Frame completo de métricas
 * ========================================================= */
typedef struct {
    int            frame;
    double         time_ms;        /* tempo total do frame */
    int            migrations;     /* total de migrações no frame */
    TreeMetrics    tree;
    ThreadMetrics  threads[MAX_THREADS];
    BalanceMetrics balance;
    int            num_threads;
} FrameMetrics;

/* =========================================================
 * Handles de arquivos CSV abertos
 * ========================================================= */
typedef struct {
    FILE *f_time;
    FILE *f_tree;
    FILE *f_threads;
    FILE *f_transfers;
    FILE *f_balance;
} CsvHandles;

/* Evita dependência circular: quadtree.h inclui types.h; metrics.h também */
struct QuadTreeNode;

/* --- API --- */
CsvHandles   metrics_open_csv(const char *dir);
void         metrics_close_csv(CsvHandles *h);
void         metrics_write_frame(CsvHandles *h, const FrameMetrics *fm);

BalanceMetrics metrics_calc_balance(const ThreadMetrics *threads, int n);
TreeMetrics    metrics_collect_tree(struct QuadTreeNode *root);

#endif /* METRICS_H */
