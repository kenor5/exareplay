#include "slot.h"

void
slot_init(exareplay_t *ctx)
{
    exanic_tx_t *tx = ctx->device->tx;
    struct tx_chunk *chunk = NULL;

    size_t padding = exanic_payload_padding_bytes(EXANIC_TX_TYPE_RAW); // 2
    for (int i = 0; i < TX_SLOT_NUM; ++i) {
        chunk = (struct tx_chunk *)(tx->buffer + TX_SLOT_SIZE * i);

        chunk->feedback_id = 0x0000;         /* Not applicable. */
        chunk->feedback_slot_index = 0x8000; /* No feedback. */
        chunk->length = padding + 0;         /* Frame size + padding. */
        chunk->type = EXANIC_TX_TYPE_RAW;    /* Only supported transmit type. */
        chunk->flags = 0;
    }

    ctx->slot_info.cap = TX_SLOT_NUM;
    ctx->slot_info.size = 0;
    ctx->slot_info.head = 0;
    ctx->slot_info.tail = 0;
}

void
set_slot_len(exanic_tx_t *tx, int slot_idx, uint16_t len)
{
    struct tx_chunk *chunk = (struct tx_chunk *)(tx->buffer + TX_SLOT_SIZE * slot_idx);
    size_t padding = exanic_payload_padding_bytes(EXANIC_TX_TYPE_RAW);
    chunk->length = padding + len;
}

/* Get a pointer to the payload for a given slot. */
char *
get_slot_payload(exanic_tx_t *tx, int slot)
{
    struct tx_chunk *chunk = (struct tx_chunk *)(tx->buffer + TX_SLOT_SIZE * slot);
    size_t padding = exanic_payload_padding_bytes(EXANIC_TX_TYPE_RAW);

    return chunk->payload + padding;
}

void
fill_slot(exareplay_t *ctx)
{
    int slot_idx = ctx->slot_info.tail;

    char *payload = get_slot_payload(ctx->device->tx, slot_idx);

    pcap_info_t *pcap_info = ringbuffer_next_use(ctx->pcap_info);

    /* move from memory to slot, increase used ptr */
    set_slot_len(ctx->device->tx, slot_idx, pcap_info->len);
    memcpy(payload, pcap_info->data, pcap_info->len);

    ringbuffer_used_inc(ctx->pcap_info);

    /* ctx->slot_info.size++ */
    uint32_t oldsize = ctx->slot_info.size;
    while (!__sync_bool_compare_and_swap(&ctx->slot_info.size, oldsize, oldsize + 1)) {
        oldsize = ctx->slot_info.size;
    }

    ctx->slot_info.tail = (ctx->slot_info.tail + 1) % ctx->slot_info.cap;
}

void
flush_wc_buffers(exanic_tx_t *tx)
{
    /*
     * This should trigger a write combining flush. It is a dummy write to a
     * read only register.
     */
    tx->exanic->registers[REG_EXANIC_INDEX(REG_EXANIC_PCIE_IF_VER)] = 0xDEADBEEF;

    /* --> this may work too: asm volatile ("sfence" ::: "memory"); */
}

void
trigger_slot_send(exareplay_t *ctx, int slot)
{
    exanic_tx_t *tx = ctx->device->tx;
    int offset = slot * TX_SLOT_SIZE;
    tx->exanic->registers[REG_PORT_INDEX(tx->port_number, REG_PORT_TX_COMMAND)] = offset + tx->buffer_offset;

    /* ctx->slot_info.size--; */
    uint32_t oldsize = ctx->slot_info.size;
    while (!__sync_bool_compare_and_swap(&ctx->slot_info.size, oldsize, oldsize - 1)) {
        oldsize = ctx->slot_info.size;
    }
    ctx->slot_info.head = (ctx->slot_info.head + 1) % ctx->slot_info.cap;
}

void
slot_preload(exareplay_t *ctx)
{
    for (int i = 0; i < TX_SLOT_NUM; ++i) {
        fill_slot(ctx);
    }

    flush_wc_buffers(ctx->device->tx);
}