#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include <stddef.h>     // For size_t
#include <stdbool.h>    // For bool type
#include "logger.h"     // For logging queue status

// ============================================================
// Queue Manager
// ------------------------------------------------------------
// Handles buffering of data packets or commands to ensure that
// incoming data can be processed without overflow. Useful for
// streaming or handling burst data from PAC-RF.
// ============================================================

// Queue element type (adjust as needed for actual data)
typedef struct {
    char data[256];     // Example payload buffer
    size_t length;      // Length of the payload
} QueueItem;

// Queue structure
typedef struct {
    QueueItem *items;   // Dynamic array of queue items
    size_t capacity;    // Maximum number of items
    size_t head;        // Read pointer
    size_t tail;        // Write pointer
    size_t count;       // Current number of items
} Queue;

// -------------------- Public API --------------------

// Initializes the queue with the given capacity
bool queue_init(Queue *q, size_t capacity);

// Frees any memory associated with the queue
void queue_destroy(Queue *q);

// Adds an item to the queue (returns false if full)
bool queue_enqueue(Queue *q, const QueueItem *item);

// Removes an item from the queue (returns false if empty)
bool queue_dequeue(Queue *q, QueueItem *out_item);

// Checks if the queue is full
bool queue_is_full(const Queue *q);

// Checks if the queue is empty
bool queue_is_empty(const Queue *q);

// Logs the current status of the queue
void queue_log_status(const Queue *q);

#endif // QUEUE_MANAGER_H
