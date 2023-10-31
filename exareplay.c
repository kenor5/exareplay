#include "exareplay.h"
#include "send_packets.h"

exareplay_t *ctx;

int
main(int argc, char *argv[])
{
    int rcode;

    fflush(NULL);

    ctx = exareplay_init();

    rcode = exareplay_parse_args(ctx, argc, argv);
    if (rcode < 0) {
        fprintf(stderr, "parse args error\n");
        return -1;
    }

    device_init(ctx);

    pcap_info_init(ctx);

    load_pcap(ctx);

    slot_init(ctx->device->tx);

    slot_preload(ctx);

    /* main loop */
    exareplay_replay(ctx);

    pcap_info_free(ctx);

    device_close(ctx);

    exareplay_free(ctx);

    return 0;
}