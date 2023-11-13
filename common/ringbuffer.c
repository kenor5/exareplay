#include "ringbuffer.h"
#include "utils.h"
#include <string.h>

#define MOD(a, b) ((a) & ((b)-1))

struct ringbuffer {
    /* length of each element */
    size_t elem_size;
    size_t capacity;
    volatile size_t size;
    size_t head;
    size_t tail;

    /* next element to move to NIC */
    size_t used;
    char *data;
};

/**
 * Create a ringbuffer
 * @param elem_size length of each element
 * @param capacity capacity of the ringbuffer
 * @return pointer to the ringbuffer
 */
ringbuffer_t *
ringbuffer_create(size_t elem_size, size_t capacity)
{
    ringbuffer_t *rb = safe_malloc(sizeof(ringbuffer_t));
    /* if capacity is not power of 2, align*/
    while (capacity & (capacity - 1)) {
        capacity = capacity & (capacity - 1);
    }

    rb->elem_size = elem_size;
    rb->capacity = capacity;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->used = 0;
    rb->data = safe_malloc(elem_size * capacity);

    return rb;
}

/**
 * Destroy a ringbuffer
 * @param rb pointer to the ringbuffer
 */
void
ringbuffer_destroy(ringbuffer_t *rb)
{
    safe_free(rb->data);
    safe_free(rb);
}

/**
 * Push an element to the ringbuffer
 * @param rb pointer to the ringbuffer
 * @param elem pointer to the element
 */
void
ringbuffer_push(ringbuffer_t *rb, void *elem)
{
    u_int32_t cap = rb->capacity;
    u_int32_t elem_size = rb->elem_size;
    memcpy(rb->data + MOD(rb->tail, cap) * rb->elem_size, elem, rb->elem_size);
    rb->tail = rb->tail + 1;

    /* atomically increase size */
    size_t size = rb->size;
    while (!__sync_bool_compare_and_swap(&rb->size, size, size + 1)) {
        size = rb->size;
    }
}

/**
 * Pop an element from the ringbuffer
 * @param rb pointer to the ringbuffer
 */
void
ringbuffer_pop(ringbuffer_t *rb)
{
    rb->head = rb->head + 1;
    /* atomically decrease size */
    size_t size = rb->size;
    while (!__sync_bool_compare_and_swap(&rb->size, size, size - 1)) {
        size = rb->size;
    }
}

/**
 * Get the capacity of the ringbuffer
 * @param rb pointer to the ringbuffer
 * @return capacity of the ringbuffer
 */
size_t
ringbuffer_capacity(ringbuffer_t *rb)
{
    return rb->capacity;
}

/**
 * Get the size of the ringbuffer
 * @param rb pointer to the ringbuffer
 * @return size of the ringbuffer
 */
size_t
ringbuffer_size(ringbuffer_t *rb)
{
    return rb->size;
}

/**
 * Get the front element of the ringbuffer
 * @param rb pointer to the ringbuffer
 * @return pointer to the front element
 */
void *
ringbuffer_front(ringbuffer_t *rb)
{
    return rb->data + MOD(rb->head, rb->capacity) * rb->elem_size;
}

void *
ringbuffer_next_use(ringbuffer_t *rb)
{
    return rb->data + MOD(rb->used, rb->capacity) * rb->elem_size;
}

/**
 * Clear the ringbuffer
 * @param rb pointer to the ringbuffer
 */
void
ringbuffer_clear(ringbuffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}

bool
ringbuffer_tofill(ringbuffer_t *rb)
{
    return rb->size < rb->capacity - 1;
}

bool
ringbuffer_empty(ringbuffer_t *rb)
{
    return rb->size <= 0;
}

void
ringbuffer_used_inc(ringbuffer_t *rb)
{
    rb->used++;
}

bool
ringbuffer_useup(ringbuffer_t *rb)
{
    return rb->used > rb->tail;
}