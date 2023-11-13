#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct ringbuffer ringbuffer_t;

ringbuffer_t *ringbuffer_create(size_t elem_size, size_t capacity);
void ringbuffer_destroy(ringbuffer_t *rb);

void ringbuffer_push(ringbuffer_t *rb, void *elem);
void ringbuffer_pop(ringbuffer_t *rb);

size_t ringbuffer_capacity(ringbuffer_t *rb);
size_t ringbuffer_size(ringbuffer_t *rb);

bool ringbuffer_tofill(ringbuffer_t *rb);

void *ringbuffer_front(ringbuffer_t *rb);

void *ringbuffer_next_use(ringbuffer_t *rb);

void ringbuffer_clear(ringbuffer_t *rb);

bool ringbuffer_empty(ringbuffer_t *rb);

void ringbuffer_used_inc(ringbuffer_t *rb);

bool ringbuffer_useup(ringbuffer_t *rb);