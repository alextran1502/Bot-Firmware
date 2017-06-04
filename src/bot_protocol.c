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

static struct udp_pcb *bot_udp;


static void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port) {
    if (p) {
        UARTprintf("Received buf, %d bytes\n", p->len);
        pbuf_free(p);
    }
}


void BotProto_SendTelemetry(void)
{
    char msg[1024];
    static int n = 0;
    int l = usnprintf(msg, sizeof msg, "%16d", ++n);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, l, PBUF_RAM);
    memcpy(p->payload, msg, l);

    struct ip_addr ip_dest;
    ip_dest.addr = htonl(settings.ip_controller);
    udp_sendto(bot_udp, p, &ip_dest, settings.udp_port);
    pbuf_free(p);
}


void BotProto_Init(void)
{
    bot_udp = udp_new();
    udp_bind(bot_udp, IP_ADDR_ANY, settings.udp_port);
    udp_recv(bot_udp, udp_received, 0);
}
