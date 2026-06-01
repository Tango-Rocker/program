#include "event/event.h"

#include <stdlib.h>
#include <string.h>

GameEventQueueResult game_event_queue_init(GameEventQueue *queue, size_t capacity) {
    if (!queue || capacity == 0) {
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    queue->events = (GameEvent *)malloc(capacity * sizeof(GameEvent));
    if (!queue->events) {
        game_event_queue_destroy(queue);
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    queue->capacity = capacity;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
    return GAME_EVENT_QUEUE_RESULT_OK;
}

void game_event_queue_destroy(GameEventQueue *queue) {
    if (!queue) {
        return;
    }

    free(queue->events);
    queue->events = NULL;
    queue->capacity = 0u;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
}

GameEventQueueResult game_event_queue_push(GameEventQueue *queue, const GameEvent *event) {
    if (!queue || !event) {
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->events || queue->capacity == 0) {
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (event->payload_size > GAME_EVENT_MAX_PAYLOAD_BYTES) {
        return GAME_EVENT_QUEUE_RESULT_BAD_PAYLOAD;
    }

    if (queue->count == queue->capacity) {
        return GAME_EVENT_QUEUE_RESULT_FULL;
    }

    queue->events[queue->tail] = *event;
    queue->tail++;
    if (queue->tail == queue->capacity) {
        queue->tail = 0;
    }
    queue->count++;
    return GAME_EVENT_QUEUE_RESULT_OK;
}

GameEventQueueResult game_event_queue_pop(GameEventQueue *queue, GameEvent *out_event) {
    if (!queue) {
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->events || queue->capacity == 0 || queue->count == 0) {
        return GAME_EVENT_QUEUE_RESULT_EMPTY;
    }

    if (out_event) {
        *out_event = queue->events[queue->head];
    }

    queue->head++;
    if (queue->head == queue->capacity) {
        queue->head = 0;
    }
    queue->count--;
    return GAME_EVENT_QUEUE_RESULT_OK;
}

GameEventQueueResult game_event_queue_peek(const GameEventQueue *queue, GameEvent *out_event) {
    if (!queue || !out_event) {
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->events || queue->capacity == 0 || queue->count == 0) {
        return GAME_EVENT_QUEUE_RESULT_EMPTY;
    }

    *out_event = queue->events[queue->head];
    return GAME_EVENT_QUEUE_RESULT_OK;
}

GameEventQueueResult game_event_queue_clear(GameEventQueue *queue) {
    if (!queue) {
        return GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->events || queue->capacity == 0) {
        queue->head = 0u;
        queue->tail = 0u;
        queue->count = 0u;
        return GAME_EVENT_QUEUE_RESULT_OK;
    }

    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
    return GAME_EVENT_QUEUE_RESULT_OK;
}

size_t game_event_queue_count(const GameEventQueue *queue) {
    return queue ? queue->count : 0u;
}
