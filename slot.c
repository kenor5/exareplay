#include "slot.h"

void slot_init(exanic_tx_t *tx) {
    struct tx_chunk *chunk = NULL;

    size_t padding = exanic_payload_padding_bytes(EXANIC_TX_TYPE_RAW); // 2
    for (int i = 0; i < TX_SLOT_NUM; ++i) {
        chunk = (struct tx_chunk *) (tx->buffer + TX_SLOT_SIZE * i);

        chunk->feedback_id = 0x0000;            /* Not applicable. */
        chunk->feedback_slot_index = 0x8000;    /* No feedback. */
        chunk->length = padding + 0 ;      /* Frame size + padding. */
        chunk->type = EXANIC_TX_TYPE_RAW;       /* Only supported transmit type. */
        chunk->flags = 0;
    }

}

void set_slot_len(exanic_tx_t *tx, int slot_idx, uint16_t len) {
    struct tx_chunk *chunk =
        (struct tx_chunk *) (tx->buffer + TX_SLOT_SIZE * slot_idx);
    size_t padding = exanic_payload_padding_bytes(EXANIC_TX_TYPE_RAW);
    chunk->length = padding + len;
}

/* Get a pointer to the payload for a given slot. */
char * get_slot_payload(exanic_tx_t *tx, int slot)
{
    struct tx_chunk *chunk =
        (struct tx_chunk *) (tx->buffer + TX_SLOT_SIZE * slot);
    size_t padding = exanic_payload_padding_bytes(EXANIC_TX_TYPE_RAW);

    return chunk->payload + padding;
}

void fill_slot(exareplay_t *ctx, int pcap_idx, int slot_idx) {
    char* payload = get_slot_payload(ctx->device->tx, slot_idx);

    set_slot_len(ctx->device->tx, slot_idx, ctx->pcap_info->len[pcap_idx]);
    memcpy(payload, &ctx->pcap_info->data[pcap_idx], ctx->pcap_info->len[pcap_idx]);
}

void flush_wc_buffers(exanic_tx_t *tx)
{
    /*
     * This should trigger a write combining flush. It is a dummy write to a
     * read only register.
     */
    tx->exanic->registers[REG_EXANIC_INDEX(REG_EXANIC_PCIE_IF_VER)]
        = 0xDEADBEEF;

    /* --> this may work too: asm volatile ("sfence" ::: "memory"); */
}

void trigger_slot_send(exanic_tx_t *tx, int slot)
{
    int offset = slot * TX_SLOT_SIZE;
    tx->exanic->registers[REG_PORT_INDEX(tx->port_number, REG_PORT_TX_COMMAND)]
        = offset + tx->buffer_offset;
}

void slot_preload(exareplay_t *ctx) {

    for (int i = 0; i < TX_SLOT_NUM; ++i) {
        fill_slot(ctx, i, i);
    }

    flush_wc_buffers(ctx->device->tx);
}