#ifndef TRANSFER_H
#define TRANSFER_H

#include "types.h"
#include <pthread.h>
#include <stdbool.h>

/* Capacidade máxima da fila de transferência por thread */
#define TRANSFER_QUEUE_CAP 512

/* =========================================================
 * Fila thread-safe de partículas entre threads
 *
 * Cada thread de destino possui uma fila. A thread de origem
 * escreve (enqueue) e a thread de destino lê (dequeue).
 * O mutex protege a fila inteira — simples e sem deadlock
 * porque cada thread só trava a fila da thread de destino.
 * ========================================================= */
typedef struct {
    Particle *items[TRANSFER_QUEUE_CAP];
    int       head;
    int       tail;
    int       count;
    pthread_mutex_t lock;
} TransferQueue;

void transfer_queue_init(TransferQueue *q);
void transfer_queue_destroy(TransferQueue *q);

/* Retorna false se a fila está cheia */
bool transfer_enqueue(TransferQueue *q, Particle *p);

/* Retorna NULL se a fila está vazia */
Particle *transfer_dequeue(TransferQueue *q);

/* Drena todos os itens para out[]; retorna quantidade */
int transfer_drain(TransferQueue *q, Particle **out, int max);

#endif /* TRANSFER_H */
