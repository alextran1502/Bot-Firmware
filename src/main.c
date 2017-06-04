
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"
#include "settings.h"
#include "bot_protocol.h"

#define SYSTICK_HZ  100

struct udp_pcb *bot_udp_pcb;


void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port) {
    if (p) {
        UARTprintf("Received buf, %d bytes\n", p->len);
        pbuf_free(p);
    }
}

void send_telemetry_packet(void)
{
    struct ip_addr ip_dest;
    ip_dest.addr = htonl(settings.ip_controller);

    char msg[1024];
    static int n = 0;
    int l = usnprintf(msg, sizeof msg, "%16d", ++n);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, l, PBUF_RAM);
    memcpy(p->payload, msg, l);

    udp_sendto(bot_udp_pcb, p, &ip_dest, BOT_UDP_PORT);
    pbuf_free(p);
}

void lwIPHostTimerHandler(void)
{
    if (!console_is_interactive) {
        uint32_t ip = lwIPLocalIPAddrGet();
        UARTprintf("IP=%d.%d.%d.%d RX=%x TX=%x\n",
            (ip >> 0) & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff,
            lwip_stats.link.recv, lwip_stats.link.xmit);
    }
}

void systick_isr(void)
{
    send_telemetry_packet();
    lwIPTimer(1000 / SYSTICK_HZ);
}

int main(void)
{
    MAP_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    uint32_t sysclock_hz = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    PinoutSet(true, false);

    UARTStdioConfig(0, 115200, sysclock_hz);
    UARTprintf("\n\n====\n"
               "Tuco Flyer Firmware starting!\n\n");

    Settings_Init();

    MAP_SysTickPeriodSet(sysclock_hz / SYSTICK_HZ);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    MAP_IntPrioritySet(INT_EMAC0,       0xC0);
    MAP_IntPrioritySet(FAULT_SYSTICK,   0x80);

    lwIPInit(sysclock_hz, mac_address,
        settings.ip_addr, settings.ip_netmask, settings.ip_gateway,
        IPADDR_USE_STATIC);

    bot_udp_pcb = udp_new();
    udp_bind(bot_udp_pcb, IP_ADDR_ANY, BOT_UDP_PORT);
    udp_recv(bot_udp_pcb, udp_received, 0);

    Settings_Console();
}
