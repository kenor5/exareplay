#include "exareplay_api.h"

/* Get a pointer to the payload for a given slot. */
char * get_slot_payload(exanic_tx_t *tx, int slot);

void set_slot_len(exanic_tx_t *tx, int slot_idx, uint16_t len);

void slot_init(exanic_tx_t *tx);

void fill_slot(exareplay_t *, int pcap_idx, int slot_idx);

/*
 * Force the write combining buffers to be flushed after pushing all of the
 * frames to the card.
 */
void flush_wc_buffers(exanic_tx_t *tx);

/* Trigger a send on a given slot. */
void trigger_slot_send(exanic_tx_t *tx, int slot);

void slot_preload(exareplay_t *);