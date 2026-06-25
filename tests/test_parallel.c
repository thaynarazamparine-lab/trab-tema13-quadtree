#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "../include/quadtree.h"
#include "../include/transfer.h"
#include "../include/metrics.h"

/* =========================================================
 * Helpers
 * ========================================================= */
static Particle make_p(int id, double x, double y) {
    Particle p = {0};
    p.id = id; p.position.x = x; p.position.y = y;
    p.radius = 1.0; p.mass = 1.0;
    return p;
}

/* =========================================================
 * PARTE 1 — Testes da fila de transferência (TransferQueue)
 * ========================================================= */

static void test_transfer_enqueue_dequeue(void) {
    TransferQueue q;
    transfer_queue_init(&q);

    Particle p = make_p(1, 10.0, 10.0);
    assert(transfer_enqueue(&q, &p) == true);

    Particle *got = transfer_dequeue(&q);
    assert(got == &p);
    assert(transfer_dequeue(&q) == NULL);  /* fila vazia */

    transfer_queue_destroy(&q);
    printf("  [OK] test_transfer_enqueue_dequeue\n");
}

static void test_transfer_full(void) {
    TransferQueue q;
    transfer_queue_init(&q);

    /* Preenche até a capacidade */
    Particle dummy[TRANSFER_QUEUE_CAP];
    for (int i = 0; i < TRANSFER_QUEUE_CAP; i++) {
        dummy[i] = make_p(i, 0.0, 0.0);
        assert(transfer_enqueue(&q, &dummy[i]) == true);
    }
    /* Próximo deve falhar */
    Particle extra = make_p(9999, 0.0, 0.0);
    assert(transfer_enqueue(&q, &extra) == false);

    transfer_queue_destroy(&q);
    printf("  [OK] test_transfer_full\n");
}

static void test_transfer_drain(void) {
    TransferQueue q;
    transfer_queue_init(&q);

    Particle p1 = make_p(1, 0.0, 0.0);
    Particle p2 = make_p(2, 0.0, 0.0);
    Particle p3 = make_p(3, 0.0, 0.0);
    transfer_enqueue(&q, &p1);
    transfer_enqueue(&q, &p2);
    transfer_enqueue(&q, &p3);

    Particle *out[10];
    int n = transfer_drain(&q, out, 10);
    assert(n == 3);
    assert(out[0] == &p1);
    assert(out[1] == &p2);
    assert(out[2] == &p3);
    assert(transfer_dequeue(&q) == NULL);

    transfer_queue_destroy(&q);
    printf("  [OK] test_transfer_drain\n");
}

/* Teste de transferência concorrente: produtor + consumidor */
typedef struct {
    TransferQueue *q;
    Particle      *items;
    int            n;
} ProducerArg;

static void *producer_thread(void *arg) {
    ProducerArg *a = (ProducerArg *)arg;
    for (int i = 0; i < a->n; i++) {
        while (!transfer_enqueue(a->q, &a->items[i]))
            ; /* espera se cheia */
    }
    return NULL;
}

static void test_transfer_concurrent(void) {
    TransferQueue q;
    transfer_queue_init(&q);

    int N = 200;
    Particle *items = malloc(N * sizeof(Particle));
    for (int i = 0; i < N; i++) items[i] = make_p(i, 0.0, 0.0);

    ProducerArg pa = {&q, items, N};
    pthread_t tid;
    pthread_create(&tid, NULL, producer_thread, &pa);

    int received = 0;
    while (received < N) {
        Particle *p = transfer_dequeue(&q);
        if (p) received++;
    }
    pthread_join(tid, NULL);
    assert(received == N);

    free(items);
    transfer_queue_destroy(&q);
    printf("  [OK] test_transfer_concurrent (%d itens)\n", N);
}

/* =========================================================
 * PARTE 2 — Testes de métricas
 * ========================================================= */

static void test_metrics_balance_equal(void) {
    ThreadMetrics threads[4] = {0};
    for (int i = 0; i < 4; i++) {
        threads[i].thread_id = i;
        threads[i].particles_processed = 50;
    }
    BalanceMetrics b = metrics_calc_balance(threads, 4);
    assert(b.max_load == 50.0);
    assert(b.min_load == 50.0);
    assert(b.avg_load == 50.0);
    assert(b.stddev_load < 1e-9);
    assert(b.imbalance  < 1e-9);
    printf("  [OK] test_metrics_balance_equal\n");
}

static void test_metrics_balance_unequal(void) {
    ThreadMetrics threads[4] = {0};
    threads[0].particles_processed = 400;
    threads[1].particles_processed = 10;
    threads[2].particles_processed = 10;
    threads[3].particles_processed = 10;
    BalanceMetrics b = metrics_calc_balance(threads, 4);
    assert(b.max_load == 400.0);
    assert(b.min_load == 10.0);
    assert(b.avg_load == 107.5);
    /* imbalance = (400 - 107.5) / 107.5 ≈ 2.72 */
    assert(b.imbalance > 2.0);
    printf("  [OK] test_metrics_balance_unequal (imbalance=%.2f)\n", b.imbalance);
}

static void test_metrics_tree_empty(void) {
    AABB bounds = {0.0, 0.0, 100.0, 100.0};
    QuadTreeNode *root = create_node(bounds);
    TreeMetrics m = metrics_collect_tree(root);
    assert(m.total_nodes == 1);
    assert(m.max_depth   == 0);
    free_tree(root);
    printf("  [OK] test_metrics_tree_empty\n");
}

static void test_metrics_tree_subdivided(void) {
    AABB bounds = {0.0, 0.0, 100.0, 100.0};
    QuadTreeNode *root = create_node(bounds);
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_p(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided);
    TreeMetrics m = metrics_collect_tree(root);
    assert(m.total_nodes > 1);
    assert(m.max_depth   >= 1);
    free_tree(root);
    printf("  [OK] test_metrics_tree_subdivided (nodes=%d, depth=%d)\n",
           m.total_nodes, m.max_depth);
}

/* =========================================================
 * PARTE 3 — Testes de exportação CSV
 * ========================================================= */

static void test_csv_export(void) {
    const char *dir = "/tmp/test_csv_quadtree";
    CsvHandles h = metrics_open_csv(dir);
    assert(h.f_time      != NULL);
    assert(h.f_tree      != NULL);
    assert(h.f_threads   != NULL);
    assert(h.f_transfers != NULL);
    assert(h.f_balance   != NULL);

    FrameMetrics fm = {0};
    fm.frame       = 0;
    fm.time_ms     = 16.5;
    fm.num_threads = 2;
    fm.migrations  = 3;
    fm.threads[0].thread_id          = 0;
    fm.threads[0].particles_processed = 100;
    fm.threads[0].time_spent_ms       = 8.2;
    fm.threads[1].thread_id          = 1;
    fm.threads[1].particles_processed = 80;
    fm.threads[1].time_spent_ms       = 8.3;
    fm.tree.total_nodes = 5;
    fm.tree.max_depth   = 2;
    fm.tree.avg_depth   = 1.5;
    fm.balance = metrics_calc_balance(fm.threads, 2);

    metrics_write_frame(&h, &fm);
    metrics_close_csv(&h);

    /* Verifica que o arquivo de tempo tem conteúdo */
    char path[256];
    snprintf(path, sizeof(path), "%s/time.csv", dir);
    FILE *f = fopen(path, "r");
    assert(f != NULL);
    char line[128];
    assert(fgets(line, sizeof(line), f) != NULL); /* cabeçalho */
    assert(fgets(line, sizeof(line), f) != NULL); /* dados */
    fclose(f);

    printf("  [OK] test_csv_export\n");
}

/* =========================================================
 * PARTE 4 — Teste de cenário de desbalanceamento
 * ========================================================= */

static void test_imbalanced_scenario_reproducible(void) {
    /* Duas execuções com a mesma seed devem gerar as mesmas posições */
    int N = 100;
    Particle *a = malloc(N * sizeof(Particle));
    Particle *b = malloc(N * sizeof(Particle));

    unsigned seed = 42;
    srand(seed);
    for (int i = 0; i < N; i++) {
        a[i].position.x = (double)(rand() % 800);
        a[i].position.y = (double)(rand() % 600);
    }

    srand(seed);
    for (int i = 0; i < N; i++) {
        b[i].position.x = (double)(rand() % 800);
        b[i].position.y = (double)(rand() % 600);
    }

    for (int i = 0; i < N; i++) {
        assert(a[i].position.x == b[i].position.x);
        assert(a[i].position.y == b[i].position.y);
    }

    free(a); free(b);
    printf("  [OK] test_imbalanced_scenario_reproducible (seed=%u)\n", seed);
}

static void test_imbalanced_concentration(void) {
    /* 80% das partículas devem estar em uma região pequena */
    int N = 500;
    int hot = (int)(N * 0.80);
    double hot_w = 80.0, hot_h = 60.0;

    Particle *ps = malloc(N * sizeof(Particle));
    srand(42);
    for (int i = 0; i < N; i++) {
        if (i < hot) {
            ps[i].position.x = (double)(rand() % (int)hot_w);
            ps[i].position.y = (double)(rand() % (int)hot_h);
        } else {
            double x, y;
            do {
                x = (double)(rand() % 796 + 2);
                y = (double)(rand() % 596 + 2);
            } while (x < hot_w && y < hot_h);
            ps[i].position.x = x;
            ps[i].position.y = y;
        }
    }

    /* Conta quantas estão na região quente */
    int in_hot = 0;
    for (int i = 0; i < N; i++) {
        if (ps[i].position.x <= hot_w && ps[i].position.y <= hot_h)
            in_hot++;
    }
    assert(in_hot >= (int)(N * 0.75)); /* pelo menos 75% */

    free(ps);
    printf("  [OK] test_imbalanced_concentration (%d/%d na regiao quente)\n", in_hot, N);
}

/* =========================================================
 * PARTE 5 — Rebalanceamento: reconstrói árvore após movimentação
 * ========================================================= */

static void test_rebalance_after_move(void) {
    AABB bounds = {0.0, 0.0, 100.0, 100.0};
    QuadTreeNode *root = create_node(bounds);

    Particle buf[MAX_PARTICLES + 1];
    double vx[MAX_PARTICLES + 1], vy[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_p(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        vx[i] = 5.0; vy[i] = 5.0;
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided);

    /* Move todas as partículas */
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i].position.x += vx[i];
        buf[i].position.y += vy[i];
    }

    /* Rebalanceamento por reconstrução */
    free_tree(root);
    root = create_node(bounds);
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        if (aabb_contains(bounds, buf[i].position.x, buf[i].position.y))
            insert_particle(root, &buf[i]);
    }

    /* Árvore deve ser consistente */
    TreeMetrics m = metrics_collect_tree(root);
    assert(m.total_nodes >= 1);

    free_tree(root);
    printf("  [OK] test_rebalance_after_move\n");
}

/* =========================================================
 * main
 * ========================================================= */
int main(void) {
    printf("\n=== Testes de Transferencia Thread-Safe ===\n");
    test_transfer_enqueue_dequeue();
    test_transfer_full();
    test_transfer_drain();
    test_transfer_concurrent();

    printf("\n=== Testes de Metricas ===\n");
    test_metrics_balance_equal();
    test_metrics_balance_unequal();
    test_metrics_tree_empty();
    test_metrics_tree_subdivided();

    printf("\n=== Testes de Exportacao CSV ===\n");
    test_csv_export();

    printf("\n=== Testes de Cenario de Desbalanceamento ===\n");
    test_imbalanced_scenario_reproducible();
    test_imbalanced_concentration();

    printf("\n=== Testes de Rebalanceamento ===\n");
    test_rebalance_after_move();

    printf("\nTodos os testes paralelos passaram.\n");
    return 0;
}
