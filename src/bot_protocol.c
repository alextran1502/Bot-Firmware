#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "settings.h"
#include "bot_protocol.h"
#include "gimbal.h"

static struct udp_pcb *bot_udp;


static void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
    uint8_t *hdr = p->payload;
    if (!p->len) {
        pbuf_free(p);
        return;
    }
    switch (hdr[0]) {

        case BOT_MSG_LOOPBACK: {
            // Reply with entire packet including header
            struct ip_addr ctrl;
            ctrl.addr = htonl(settings.ip_controller);
            udp_sendto(bot_udp, p, &ctrl, settings.udp_port);
            break;
        }

        case BOT_MSG_GIMBAL: {
            pbuf_header(p, -1);
            Gimbal_TxQueue(p);
            break;
        }

    }
    pbuf_free(p);
}


void BotProto_Send(struct pbuf *p)
{
    struct ip_addr ctrl;
    ctrl.addr = htonl(settings.ip_controller);
    udp_sendto(bot_udp, p, &ctrl, settings.udp_port);
}


void BotProto_Telemetry(void)
{
    if (settings.debug_flags & DBGF_NO_TELEMETRY) {
        return;
    }

    char msg[1024];
    static int n = 0;
    int l = usnprintf(msg, sizeof msg, "%16d", ++n);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, l, PBUF_RAM);
    memcpy(p->payload, msg, l);

    BotProto_Send(p);
    pbuf_free(p);
}


void BotProto_Init(void)
{
    bot_udp = udp_new();
    udp_bind(bot_udp, IP_ADDR_ANY, settings.udp_port);
    udp_recv(bot_udp, udp_received, 0);
}
