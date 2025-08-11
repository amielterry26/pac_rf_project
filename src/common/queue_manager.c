#include <stdio.h>
#include <stdlib.h>     // For malloc/free
#include <string.h>     // For memcpy
#include "queue_manager.h"

// ============================================================
// Queue Initialization
// ============================================================
bool queue_init(Queue *q, size_t capacity) {
    if (!q || capacity == 0) return false;

    q->items = (QueueItem *)malloc(sizeof(QueueItem) * capacity);
    if (!q->items) return false;

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    log_info("Queue initialized successfully.");
    return true;
}

// ============================================================
// Queue Destruction
// ============================================================
void queue_destroy(Queue *q) {
    if (!q || !q->items) return;

    free(q->items);
    q->items = NULL;
    q->capacity = 0;
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    log_info("Queue destroyed and memory freed.");
}

// ============================================================
// Queue Enqueue
// ============================================================
bool queue_enqueue(Queue *q, const QueueItem *item) {
    if (!q || !item) return false;
    if (queue_is_full(q)) {
        log_warning("Queue is full! Cannot enqueue new item.");
        return false;
    }

    // Copy item into queue
    q->items[q->tail] = *item;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    log_info("Item enqueued successfully.");
    return true;
}

// ============================================================
// Queue Dequeue
// ============================================================
bool queue_dequeue(Queue *q, QueueItem *out_item) {
    if (!q || !out_item) return false;
    if (queue_is_empty(q)) {
        log_warning("Queue is empty! Cannot dequeue.");
        return false;
    }

    // Copy item out
    *out_item = q->items[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    log_info("Item dequeued successfully.");
    return true;
}

// ============================================================
// Queue Status Helpers
// ============================================================
bool queue_is_full(const Queue *q) {
    return q && (q->count == q->capacity);
}

bool queue_is_empty(const Queue *q) {
    return !q || (q->count == 0);
}

// ============================================================
// Queue Logging
// ============================================================
void queue_log_status(const Queue *q) {
    if (!q) {
        log_error("Queue is NULL.");
        return;
    }

    char status[128];
    snprintf(status, sizeof(status),
             "Queue Status -> Count: %zu / %zu | Head: %zu | Tail: %zu",
             q->count, q->capacity, q->head, q->tail);
    log_info(status);
}
