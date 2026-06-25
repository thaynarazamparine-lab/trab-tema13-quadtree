#include "../include/metrics.h"
#include "../include/quadtree.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* =========================================================
 * Garante que o diretório existe (cria recursivamente)
 * ========================================================= */
static void ensure_dir(const char *path) {
    /* mkdir -p equivalente: ignora erro se já existe */
    mkdir(path, 0755);
}

/* =========================================================
 * Abre os cinco arquivos CSV com cabeçalhos
 * ========================================================= */
CsvHandles metrics_open_csv(const char *dir) {
    char path[512];
    CsvHandles h;
    memset(&h, 0, sizeof(h));

    ensure_dir(dir);

    snprintf(path, sizeof(path), "%s/time.csv", dir);
    h.f_time = fopen(path, "w");
    if (h.f_time) fprintf(h.f_time, "frame,time_ms\n");

    snprintf(path, sizeof(path), "%s/tree.csv", dir);
    h.f_tree = fopen(path, "w");
    if (h.f_tree) fprintf(h.f_tree, "frame,total_nodes,max_depth,avg_depth\n");

    snprintf(path, sizeof(path), "%s/threads.csv", dir);
    h.f_threads = fopen(path, "w");
    if (h.f_threads)
        fprintf(h.f_threads,
                "frame,thread_id,particles_processed,time_spent_ms,"
                "migrations_sent,migrations_received,"
                "insertions,removals,subdivisions,merges\n");

    snprintf(path, sizeof(path), "%s/transfers.csv", dir);
    h.f_transfers = fopen(path, "w");
    if (h.f_transfers) fprintf(h.f_transfers, "frame,migrations\n");

    snprintf(path, sizeof(path), "%s/balance.csv", dir);
    h.f_balance = fopen(path, "w");
    if (h.f_balance)
        fprintf(h.f_balance,
                "frame,max_load,min_load,avg_load,stddev_load,imbalance\n");

    return h;
}

void metrics_close_csv(CsvHandles *h) {
    if (h->f_time)      fclose(h->f_time);
    if (h->f_tree)      fclose(h->f_tree);
    if (h->f_threads)   fclose(h->f_threads);
    if (h->f_transfers) fclose(h->f_transfers);
    if (h->f_balance)   fclose(h->f_balance);
    memset(h, 0, sizeof(*h));
}

/* =========================================================
 * Calcula métricas de balanceamento a partir das threads
 * ========================================================= */
BalanceMetrics metrics_calc_balance(const ThreadMetrics *threads, int n) {
    BalanceMetrics b = {0};
    if (n <= 0) return b;

    b.max_load = (double)threads[0].particles_processed;
    b.min_load = (double)threads[0].particles_processed;
    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        double load = (double)threads[i].particles_processed;
        if (load > b.max_load) b.max_load = load;
        if (load < b.min_load) b.min_load = load;
        sum += load;
    }
    b.avg_load = sum / (double)n;

    double var = 0.0;
    for (int i = 0; i < n; i++) {
        double d = (double)threads[i].particles_processed - b.avg_load;
        var += d * d;
    }
    b.stddev_load = sqrt(var / (double)n);
    b.imbalance   = (b.avg_load > 0.0)
                    ? (b.max_load - b.avg_load) / b.avg_load
                    : 0.0;

    return b;
}

/* =========================================================
 * Escreve uma linha em cada CSV para o frame atual
 * ========================================================= */
void metrics_write_frame(CsvHandles *h, const FrameMetrics *fm) {
    if (h->f_time)
        fprintf(h->f_time, "%d,%.3f\n", fm->frame, fm->time_ms);

    if (h->f_tree)
        fprintf(h->f_tree, "%d,%d,%d,%.2f\n",
                fm->frame,
                fm->tree.total_nodes,
                fm->tree.max_depth,
                fm->tree.avg_depth);

    if (h->f_threads) {
        for (int i = 0; i < fm->num_threads; i++) {
            const ThreadMetrics *t = &fm->threads[i];
            fprintf(h->f_threads, "%d,%d,%d,%.3f,%d,%d,%d,%d,%d,%d\n",
                    fm->frame,
                    t->thread_id,
                    t->particles_processed,
                    t->time_spent_ms,
                    t->migrations_sent,
                    t->migrations_received,
                    t->insertions,
                    t->removals,
                    t->subdivisions,
                    t->merges);
        }
    }

    if (h->f_transfers)
        fprintf(h->f_transfers, "%d,%d\n", fm->frame, fm->migrations);

    if (h->f_balance) {
        const BalanceMetrics *b = &fm->balance;
        fprintf(h->f_balance, "%d,%.2f,%.2f,%.2f,%.4f,%.4f\n",
                fm->frame,
                b->max_load, b->min_load, b->avg_load,
                b->stddev_load, b->imbalance);
    }
}

/* =========================================================
 * Percorre a Quadtree e coleta métricas estruturais
 * ========================================================= */
static void walk_tree(QuadTreeNode *node, int depth,
                      int *total_nodes, int *max_depth,
                      long *depth_sum, int *leaf_count) {
    if (!node) return;
    (*total_nodes)++;
    if (depth > *max_depth) *max_depth = depth;

    if (!node->is_divided) {
        (*leaf_count)++;
        *depth_sum += depth;
        return;
    }
    walk_tree(node->nw, depth + 1, total_nodes, max_depth, depth_sum, leaf_count);
    walk_tree(node->ne, depth + 1, total_nodes, max_depth, depth_sum, leaf_count);
    walk_tree(node->sw, depth + 1, total_nodes, max_depth, depth_sum, leaf_count);
    walk_tree(node->se, depth + 1, total_nodes, max_depth, depth_sum, leaf_count);
}

TreeMetrics metrics_collect_tree(QuadTreeNode *root) {
    TreeMetrics m = {0};
    int leaf_count = 0;
    long depth_sum = 0;
    walk_tree(root, 0, &m.total_nodes, &m.max_depth, &depth_sum, &leaf_count);
    m.avg_depth = (leaf_count > 0) ? (double)depth_sum / (double)leaf_count : 0.0;
    return m;
}
