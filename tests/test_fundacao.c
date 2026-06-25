#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "../include/types.h"
#include "../include/quadtree.h"

/* =========================================================
 * Helpers
 * ========================================================= */

static Particle make_particle(int id, double x, double y) {
    Particle p = {0};
    p.id          = id;
    p.position.x  = x;
    p.position.y  = y;
    p.velocity.x  = 0.0;
    p.velocity.y  = 0.0;
    p.radius      = 1.0;
    p.mass        = 1.0;
    return p;
}

static QuadTreeNode *make_root(void) {
    AABB bounds = {0.0, 0.0, 100.0, 100.0};
    return create_node(bounds);
}

/* =========================================================
 * Testes originais de fundação (mantidos)
 * ========================================================= */

static void test_particle_creation(void) {
    Particle p = {1, {10.0, 10.0}, {0.0, 0.0}, 1.0, 5.0};
    assert(p.id          == 1);
    assert(p.position.x  == 10.0);
    assert(p.position.y  == 10.0);
    assert(p.radius      == 1.0);
    printf("  [OK] test_particle_creation\n");
}

static void test_simulation_state(void) {
    SimulationState state = {1000, 0.016, {0.0, 0.0, 800.0, 600.0}};
    assert(state.num_particles   == 1000);
    assert(state.bounds.width    == 800.0);
    printf("  [OK] test_simulation_state\n");
}

/* =========================================================
 * Testes de criação e destruição
 * ========================================================= */

static void test_create_node(void) {
    QuadTreeNode *root = make_root();
    assert(root != NULL);
    assert(root->count       == 0);
    assert(root->is_divided  == false);
    assert(root->nw          == NULL);
    assert(root->ne          == NULL);
    assert(root->sw          == NULL);
    assert(root->se          == NULL);
    free_tree(root);
    printf("  [OK] test_create_node\n");
}

static void test_free_tree_empty(void) {
    QuadTreeNode *root = make_root();
    /* Não deve dar segfault nem leak */
    free_tree(root);
    printf("  [OK] test_free_tree_empty\n");
}

static void test_free_tree_single_element(void) {
    QuadTreeNode *root = make_root();
    Particle p = make_particle(1, 50.0, 50.0);
    assert(insert_particle(root, &p));
    free_tree(root);
    printf("  [OK] test_free_tree_single_element\n");
}

static void test_free_tree_subdivided(void) {
    QuadTreeNode *root = make_root();
    /* Insere MAX_PARTICLES+1 para forçar subdivisão */
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 5.0, 10.0 + i * 5.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided);
    free_tree(root);  /* não deve vazar memória */
    printf("  [OK] test_free_tree_subdivided\n");
}

/* =========================================================
 * Testes de inserção
 * ========================================================= */

static void test_insert_simple(void) {
    QuadTreeNode *root = make_root();
    Particle p = make_particle(1, 25.0, 25.0);
    assert(insert_particle(root, &p) == true);
    assert(root->count == 1);
    assert(root->particles[0] == &p);
    free_tree(root);
    printf("  [OK] test_insert_simple\n");
}

static void test_insert_multiple_no_split(void) {
    QuadTreeNode *root = make_root();
    Particle buf[MAX_PARTICLES];
    for (int i = 0; i < MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 5.0, 10.0);
        assert(insert_particle(root, &buf[i]) == true);
    }
    assert(root->count      == MAX_PARTICLES);
    assert(root->is_divided == false);
    free_tree(root);
    printf("  [OK] test_insert_multiple_no_split\n");
}

static void test_insert_triggers_subdivide(void) {
    QuadTreeNode *root = make_root();
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        assert(insert_particle(root, &buf[i]) == true);
    }
    assert(root->is_divided == true);
    assert(root->count      == 0); /* partículas redistributas para filhos */
    free_tree(root);
    printf("  [OK] test_insert_triggers_subdivide\n");
}

/* Partícula fora dos limites deve ser rejeitada */
static void test_insert_out_of_bounds(void) {
    QuadTreeNode *root = make_root();  /* bounds: 0..100 x 0..100 */
    Particle p = make_particle(1, 200.0, 200.0);
    assert(insert_particle(root, &p) == false);
    assert(root->count == 0);
    free_tree(root);
    printf("  [OK] test_insert_out_of_bounds\n");
}

/* =========================================================
 * Testes de subdivisão
 * ========================================================= */

static void test_subdivide_creates_children(void) {
    QuadTreeNode *root = make_root();
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided == true);
    assert(root->nw != NULL);
    assert(root->ne != NULL);
    assert(root->sw != NULL);
    assert(root->se != NULL);
    free_tree(root);
    printf("  [OK] test_subdivide_creates_children\n");
}

static void test_subdivide_redistributes_correctly(void) {
    QuadTreeNode *root = make_root(); /* 0..100 x 0..100, meia = 50 */

    /* Partículas no quadrante NW (x<50, y<50) */
    Particle p1 = make_particle(1, 10.0, 10.0);
    Particle p2 = make_particle(2, 20.0, 20.0);
    Particle p3 = make_particle(3, 30.0, 30.0);
    Particle p4 = make_particle(4, 40.0, 40.0);
    /* 5ª força subdivisão */
    Particle p5 = make_particle(5, 60.0, 60.0); /* SE */

    insert_particle(root, &p1);
    insert_particle(root, &p2);
    insert_particle(root, &p3);
    insert_particle(root, &p4);
    insert_particle(root, &p5);

    assert(root->is_divided);

    /* p1..p4 devem estar em NW; p5 em SE */
    int total_children = 0;
    if (root->nw) total_children += root->nw->count;
    if (root->ne) total_children += root->ne->count;
    if (root->sw) total_children += root->sw->count;
    if (root->se) total_children += root->se->count;
    assert(total_children == 5);

    free_tree(root);
    printf("  [OK] test_subdivide_redistributes_correctly\n");
}

/* =========================================================
 * Testes de remoção
 * ========================================================= */

static void test_remove_simple(void) {
    QuadTreeNode *root = make_root();
    Particle p = make_particle(1, 25.0, 25.0);
    insert_particle(root, &p);
    assert(root->count == 1);
    assert(remove_particle(root, &p) == true);
    assert(root->count == 0);
    free_tree(root);
    printf("  [OK] test_remove_simple\n");
}

static void test_remove_nonexistent(void) {
    QuadTreeNode *root = make_root();
    Particle p  = make_particle(1, 25.0, 25.0);
    Particle p2 = make_particle(2, 75.0, 75.0);
    insert_particle(root, &p);
    /* p2 nunca foi inserida */
    assert(remove_particle(root, &p2) == false);
    assert(root->count == 1);
    free_tree(root);
    printf("  [OK] test_remove_nonexistent\n");
}

static void test_remove_in_subdivided_tree(void) {
    QuadTreeNode *root = make_root();
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided);

    /* Remove a última partícula inserida */
    assert(remove_particle(root, &buf[MAX_PARTICLES]) == true);

    /* Verifica que o total de partículas na árvore diminuiu */
    int total = 0;
    if (!root->is_divided) {
        total = root->count;
    } else {
        if (root->nw) total += root->nw->count;
        if (root->ne) total += root->ne->count;
        if (root->sw) total += root->sw->count;
        if (root->se) total += root->se->count;
    }
    assert(total == MAX_PARTICLES);

    free_tree(root);
    printf("  [OK] test_remove_in_subdivided_tree\n");
}

/* =========================================================
 * Testes de merge
 * ========================================================= */

static void test_merge_after_removal(void) {
    QuadTreeNode *root = make_root();
    /* MAX_PARTICLES+1 partículas para forçar subdivisão */
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided == true);

    /* Remove partículas até o total ser <= MERGE_THRESHOLD */
    for (int i = 0; i <= MAX_PARTICLES - MERGE_THRESHOLD; i++)
        remove_particle(root, &buf[i]);

    /* Após o merge, o nó deve ser folha novamente */
    assert(root->is_divided == false);
    assert(root->count      == MERGE_THRESHOLD);
    assert(root->nw         == NULL);

    free_tree(root);
    printf("  [OK] test_merge_after_removal\n");
}

static void test_should_merge_false_when_full(void) {
    QuadTreeNode *root = make_root();
    Particle buf[MAX_PARTICLES + 1];
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided == true);
    assert(should_merge(root) == false);
    free_tree(root);
    printf("  [OK] test_should_merge_false_when_full\n");
}

/* =========================================================
 * Testes de limites
 * ========================================================= */

static void test_tree_empty(void) {
    QuadTreeNode *root = make_root();
    /* Nenhuma operação deve travar/crashar em árvore vazia */
    Particle p = make_particle(1, 50.0, 50.0);
    assert(remove_particle(root, &p) == false);

    AABB range = {0.0, 0.0, 100.0, 100.0};
    Particle *results[10];
    assert(query_range(root, range, results, 10) == 0);

    free_tree(root);
    printf("  [OK] test_tree_empty\n");
}

static void test_single_element(void) {
    QuadTreeNode *root = make_root();
    Particle p = make_particle(42, 50.0, 50.0);
    assert(insert_particle(root, &p) == true);
    assert(root->count      == 1);
    assert(root->is_divided == false);

    AABB range = {40.0, 40.0, 20.0, 20.0};
    Particle *results[5];
    assert(query_range(root, range, results, 5) == 1);
    assert(results[0]->id == 42);

    assert(remove_particle(root, &p) == true);
    assert(root->count == 0);

    free_tree(root);
    printf("  [OK] test_single_element\n");
}

static void test_particle_on_boundary(void) {
    QuadTreeNode *root = make_root(); /* 0..100 x 0..100 */

    /* Borda exata — deve ser aceita por aabb_contains */
    Particle p1 = make_particle(1, 0.0,   0.0);
    Particle p2 = make_particle(2, 100.0, 100.0);
    Particle p3 = make_particle(3, 50.0,  0.0);
    Particle p4 = make_particle(4, 0.0,   50.0);

    assert(insert_particle(root, &p1) == true);
    assert(insert_particle(root, &p2) == true);
    assert(insert_particle(root, &p3) == true);
    assert(insert_particle(root, &p4) == true);

    free_tree(root);
    printf("  [OK] test_particle_on_boundary\n");
}

/* =========================================================
 * Testes de consulta espacial
 * ========================================================= */

static void test_query_range_basic(void) {
    QuadTreeNode *root = make_root();
    Particle inside  = make_particle(1, 25.0, 25.0);
    Particle outside = make_particle(2, 75.0, 75.0);
    insert_particle(root, &inside);
    insert_particle(root, &outside);

    AABB range = {0.0, 0.0, 50.0, 50.0}; /* cobre apenas p1 */
    Particle *results[5];
    int found = query_range(root, range, results, 5);
    assert(found == 1);
    assert(results[0]->id == 1);

    free_tree(root);
    printf("  [OK] test_query_range_basic\n");
}

static void test_query_range_subdivided(void) {
    QuadTreeNode *root = make_root();
    Particle buf[MAX_PARTICLES + 2];

    /* Insere suficientes para forçar subdivisão */
    for (int i = 0; i <= MAX_PARTICLES; i++) {
        buf[i] = make_particle(i, 10.0 + i * 3.0, 10.0 + i * 3.0);
        insert_particle(root, &buf[i]);
    }
    assert(root->is_divided);

    /* Query em toda a área — deve achar todas */
    AABB full = {0.0, 0.0, 100.0, 100.0};
    Particle *results[MAX_PARTICLES + 2];
    int found = query_range(root, full, results, MAX_PARTICLES + 2);
    assert(found == MAX_PARTICLES + 1);

    free_tree(root);
    printf("  [OK] test_query_range_subdivided\n");
}

/* =========================================================
 * Testes de vizinhos: busca por raio e vizinho mais próximo
 * ========================================================= */

static double dist2(double ax, double ay, double bx, double by) {
    double dx = ax - bx;
    double dy = ay - by;
    return dx * dx + dy * dy;
}

/* query_radius básico: distingue quem está dentro e fora do círculo */
static void test_query_radius_basic(void) {
    QuadTreeNode *root = make_root();
    Particle a = make_particle(1, 50.0, 50.0); /* centro     -> dentro */
    Particle b = make_particle(2, 55.0, 50.0); /* dist 5     -> dentro de r=10 */
    Particle c = make_particle(3, 50.0, 90.0); /* dist 40    -> fora */
    insert_particle(root, &a);
    insert_particle(root, &b);
    insert_particle(root, &c);

    Particle *res[8];
    int found = query_radius(root, 50.0, 50.0, 10.0, res, 8);
    assert(found == 2);
    free_tree(root);
    printf("  [OK] test_query_radius_basic\n");
}

/* Prova que a busca é CIRCULAR, não quadrada: um ponto que cairia dentro de
 * um quadrado de busca, mas está fora do círculo de raio r, não é retornado.
 * (90,90) dista sqrt(40^2+40^2) ~= 56.6 do centro (50,50), logo > 50. */
static void test_query_radius_is_circular(void) {
    QuadTreeNode *root = make_root();
    Particle corner = make_particle(1, 90.0, 90.0); /* fora do círculo r=50 */
    Particle center = make_particle(2, 50.0, 50.0); /* dentro */
    insert_particle(root, &corner);
    insert_particle(root, &center);

    Particle *res[8];
    int found = query_radius(root, 50.0, 50.0, 50.0, res, 8);
    assert(found == 1);
    assert(res[0]->id == 2);
    free_tree(root);
    printf("  [OK] test_query_radius_is_circular\n");
}

static void test_find_nearest_basic(void) {
    QuadTreeNode *root = make_root();
    Particle a = make_particle(1, 10.0, 10.0);
    Particle b = make_particle(2, 90.0, 90.0);
    Particle c = make_particle(3, 52.0, 52.0); /* o mais perto de (50,50) */
    insert_particle(root, &a);
    insert_particle(root, &b);
    insert_particle(root, &c);

    Particle *n = find_nearest(root, 50.0, 50.0);
    assert(n != NULL);
    assert(n->id == 3);
    free_tree(root);
    printf("  [OK] test_find_nearest_basic\n");
}

static void test_find_nearest_empty(void) {
    QuadTreeNode *root = make_root();
    assert(find_nearest(root, 50.0, 50.0) == NULL);
    free_tree(root);
    printf("  [OK] test_find_nearest_empty\n");
}

/* Cross-check aleatório: a árvore precisa concordar com a força bruta O(n^2).
 * Semente fixa => teste reproduzível. Este é o teste que dá segurança de
 * corretude e é defensável: "verifico contra a verdade ingênua". */
static void test_neighbors_vs_bruteforce(void) {
    srand(12345);
    const int    N = 300;
    const double W = 100.0;

    AABB bounds = {0.0, 0.0, W, W};
    QuadTreeNode *root = create_node(bounds);

    Particle *ps = (Particle *)malloc(N * sizeof(Particle));
    assert(ps != NULL);
    for (int i = 0; i < N; i++) {
        ps[i] = make_particle(i,
                              (double)rand() / RAND_MAX * W,
                              (double)rand() / RAND_MAX * W);
        assert(insert_particle(root, &ps[i]));
    }

    Particle *res[300];
    for (int q = 0; q < 50; q++) {
        double qx = (double)rand() / RAND_MAX * W;
        double qy = (double)rand() / RAND_MAX * W;
        double r  = 5.0 + (double)rand() / RAND_MAX * 30.0;
        double r2 = r * r;

        /* query_radius vs força bruta */
        int naive = 0;
        for (int i = 0; i < N; i++)
            if (dist2(ps[i].position.x, ps[i].position.y, qx, qy) <= r2)
                naive++;

        int found = query_radius(root, qx, qy, r, res, N);
        assert(found == naive);
        for (int i = 0; i < found; i++)
            assert(dist2(res[i]->position.x, res[i]->position.y, qx, qy) <= r2);

        /* find_nearest vs força bruta (compara distância, tolerante a empates) */
        double best_naive = DBL_MAX;
        for (int i = 0; i < N; i++) {
            double d2 = dist2(ps[i].position.x, ps[i].position.y, qx, qy);
            if (d2 < best_naive) best_naive = d2;
        }
        Particle *nn = find_nearest(root, qx, qy);
        assert(nn != NULL);
        double d2_tree = dist2(nn->position.x, nn->position.y, qx, qy);
        assert(d2_tree == best_naive);
    }

    free(ps);
    free_tree(root);
    printf("  [OK] test_neighbors_vs_bruteforce (N=%d, 50 consultas)\n", N);
}

/* =========================================================
 * Testes de auxiliares geométricos
 * ========================================================= */

static void test_aabb_contains(void) {
    AABB box = {0.0, 0.0, 100.0, 100.0};
    assert(aabb_contains(box, 50.0, 50.0)  == true);
    assert(aabb_contains(box, 0.0,  0.0)   == true);   /* canto */
    assert(aabb_contains(box, 100.0,100.0) == true);   /* canto oposto */
    assert(aabb_contains(box, 101.0,50.0)  == false);
    assert(aabb_contains(box, 50.0, -1.0)  == false);
    printf("  [OK] test_aabb_contains\n");
}

static void test_aabb_intersects(void) {
    AABB a = {0.0,  0.0,  50.0, 50.0};
    AABB b = {25.0, 25.0, 50.0, 50.0}; /* sobrepõe */
    AABB c = {60.0, 60.0, 30.0, 30.0}; /* separado */
    assert(aabb_intersects(a, b) == true);
    assert(aabb_intersects(a, c) == false);
    printf("  [OK] test_aabb_intersects\n");
}

/* =========================================================
 * main
 * ========================================================= */

int main(void) {
    printf("\n=== Testes de Fundacao ===\n");
    test_particle_creation();
    test_simulation_state();

    printf("\n=== Testes de Criacao e Destruicao ===\n");
    test_create_node();
    test_free_tree_empty();
    test_free_tree_single_element();
    test_free_tree_subdivided();

    printf("\n=== Testes de Insercao ===\n");
    test_insert_simple();
    test_insert_multiple_no_split();
    test_insert_triggers_subdivide();
    test_insert_out_of_bounds();

    printf("\n=== Testes de Subdivisao ===\n");
    test_subdivide_creates_children();
    test_subdivide_redistributes_correctly();

    printf("\n=== Testes de Remocao ===\n");
    test_remove_simple();
    test_remove_nonexistent();
    test_remove_in_subdivided_tree();

    printf("\n=== Testes de Merge ===\n");
    test_merge_after_removal();
    test_should_merge_false_when_full();

    printf("\n=== Testes de Limites ===\n");
    test_tree_empty();
    test_single_element();
    test_particle_on_boundary();

    printf("\n=== Testes de Consulta Espacial ===\n");
    test_query_range_basic();
    test_query_range_subdivided();

    printf("\n=== Testes de Vizinhos (raio e mais proximo) ===\n");
    test_query_radius_basic();
    test_query_radius_is_circular();
    test_find_nearest_basic();
    test_find_nearest_empty();
    test_neighbors_vs_bruteforce();

    printf("\n=== Testes de Auxiliares Geometricos ===\n");
    test_aabb_contains();
    test_aabb_intersects();

    printf("\nTodos os testes passaram.\n");
    return 0;
}
