#include "../include/transfer.h"
#include <stdlib.h>

void transfer_queue_init(TransferQueue *q) {
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
}

void transfer_queue_destroy(TransferQueue *q) {
    pthread_mutex_destroy(&q->lock);
}

/* Insere no final da fila circular; retorna false se cheia */
bool transfer_enqueue(TransferQueue *q, Particle *p) {
    pthread_mutex_lock(&q->lock);
    if (q->count >= TRANSFER_QUEUE_CAP) {
        pthread_mutex_unlock(&q->lock);
        return false;
    }
    q->items[q->tail] = p;
    q->tail = (q->tail + 1) % TRANSFER_QUEUE_CAP;
    q->count++;
    pthread_mutex_unlock(&q->lock);
    return true;
}

/* Remove da frente da fila; retorna NULL se vazia */
Particle *transfer_dequeue(TransferQueue *q) {
    pthread_mutex_lock(&q->lock);
    if (q->count == 0) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    Particle *p = q->items[q->head];
    q->head = (q->head + 1) % TRANSFER_QUEUE_CAP;
    q->count--;
    pthread_mutex_unlock(&q->lock);
    return p;
}

/* Drena todos os itens de uma só vez (um único lock) */
int transfer_drain(TransferQueue *q, Particle **out, int max) {
    pthread_mutex_lock(&q->lock);
    int n = 0;
    while (q->count > 0 && n < max) {
        out[n++] = q->items[q->head];
        q->head  = (q->head + 1) % TRANSFER_QUEUE_CAP;
        q->count--;
    }
    pthread_mutex_unlock(&q->lock);
    return n;
}
