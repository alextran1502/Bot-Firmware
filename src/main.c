
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/interrupt.h"
#include "driverlib/flash.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"

#define BOT_UDP_PORT            9024
#define SYSCTL_OSC_SETTING      (SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480)
#define SYSTICK_HZ              100
#define SYSTICK_INT_PRIORITY    0x80
#define ETHERNET_INT_PRIORITY   0xC0

struct udp_pcb *bot_udp_pcb;


void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port) {
    if (p) {
        UARTprintf("Received buf, %d bytes\n", p->len);
        pbuf_free(p);
    }
}


int main(void)
{
    MAP_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    uint32_t sysclock_hz = MAP_SysCtlClockFreqSet(SYSCTL_OSC_SETTING, 120000000);

    PinoutSet(true, false);

    UARTStdioConfig(0, 115200, sysclock_hz);
    UARTprintf("\n\n====\nTuco Flyer Firmware starting!\n\n");

    // Unpack and check stored MAC address
    uint32_t id0, id1;
    MAP_FlashUserGet(&id0, &id1);
    if (id0 == 0xffffffff || id1 == 0xffffffff) {
        UARTprintf("ERROR: Missing factory-programmed MAC address\n");
        while (1);
    }
    uint8_t mac[8] = {
        (id0 >>  0) & 0xff,
        (id0 >>  8) & 0xff,
        (id0 >> 16) & 0xff,
        (id1 >>  0) & 0xff,
        (id1 >>  8) & 0xff,
        (id1 >> 16) & 0xff,
        0, 0
    };

    MAP_SysTickPeriodSet(sysclock_hz / SYSTICK_HZ);
    MAP_SysTickEnable();
    MAP_SysTickIntEnable();

    MAP_IntPrioritySet(INT_EMAC0, ETHERNET_INT_PRIORITY);
    MAP_IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);

    // Static IP based on per-bot configuration
    uint32_t ip = 0x0a20000a;

    lwIPInit(sysclock_hz, mac, ip, 0xFFFFFF00, IPADDR_NONE, IPADDR_USE_STATIC);

    bot_udp_pcb = udp_new();
    udp_bind(bot_udp_pcb, IP_ADDR_ANY, BOT_UDP_PORT);
    udp_recv(bot_udp_pcb, udp_received, 0);

    return 0;
}

void lwIPHostTimerHandler(void)
{
    UARTprintf("IP=%08x RX=%x TX=%x\n", lwIPLocalIPAddrGet(), lwip_stats.link.recv, lwip_stats.link.xmit);

    char msg[] = "hiya";
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof msg, PBUF_RAM);
    memcpy(p->payload, msg, sizeof msg);
    udp_sendto(bot_udp_pcb, p, IP_ADDR_BROADCAST, BOT_UDP_PORT);
    pbuf_free(p);
}

void systick_isr(void)
{
    lwIPTimer(1000 / SYSTICK_HZ);
}

#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line)
{
    UARTprintf("ERROR at %s:%d\n", pcFilename, ui32Line);
    while (1);
}
#endif
