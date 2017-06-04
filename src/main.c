
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/interrupt.h"
#include "driverlib/flash.h"
#include "driverlib/eeprom.h"
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
#define EEPROM_OFFSET           0

struct udp_pcb *bot_udp_pcb;
volatile bool console_is_interactive = false;

#define BOT_HAS_GIMBAL          (1 << 0)
#define BOT_HAS_WINCH           (1 << 1)

struct {
    uint32_t ip_addr;
    uint32_t ip_netmask;
    uint32_t ip_gateway;
    uint32_t bot_options;
} eeprom_data;


void udp_received(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port) {
    if (p) {
        UARTprintf("Received buf, %d bytes\n", p->len);
        pbuf_free(p);
    }
}

void send_telemetry_packet(void)
{
    char msg[] = "hiya";
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof msg, PBUF_RAM);
    memcpy(p->payload, msg, sizeof msg);
    udp_sendto(bot_udp_pcb, p, IP_ADDR_BROADCAST, BOT_UDP_PORT);
    pbuf_free(p);
}

static void reboot(void)
{
    HWREG(NVIC_APINT) = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}

static void complete_setup(void)
{
    MAP_EEPROMProgram((uint32_t*) &eeprom_data, EEPROM_OFFSET, sizeof eeprom_data);
    reboot();
}

static void setup_flyer(void)
{
    UARTprintf("Setting up flyer\n");
    eeprom_data.ip_addr = 0x0a200008;
    eeprom_data.ip_netmask = 0xffffff00;
    eeprom_data.ip_gateway = 0;
    eeprom_data.bot_options = BOT_HAS_GIMBAL;
    complete_setup();
}

static void setup_winch(int number)
{
    UARTprintf("Setting up winch #%d\n", number);
    eeprom_data.ip_addr = 0x0a20000a + number;
    eeprom_data.ip_netmask = 0xffffff00;
    eeprom_data.ip_gateway = 0;
    eeprom_data.bot_options = BOT_HAS_WINCH;
    complete_setup();
}

void console_command(char *line)
{
    char *tokens;
    char *delim = " \t";
    char *command = strtok_r(line, delim, &tokens);

    if (!command || !*command) {
        return;
    }

    if (!strcmp(command, "exit")) {
        console_is_interactive = false;
        return;
    }

    if (!strcmp(command, "reboot")) {
        reboot();
        return;
    }

    if (!strcmp(command, "setwinch")) {
        char *endptr;
        char *arg = strtok_r(0, delim, &tokens);
        if (arg) {
            int number = strtol(arg, &endptr, 0);
            if (!*endptr) {
                setup_winch(number);
                return;
            }
        }
    }

    if (!strcmp(command, "setflyer")) {
        UARTprintf("Setting up flyer\n");
        setup_flyer();
        return;
    }

    if (!strcmp(command, "show")) {
        for (unsigned n = 0; n < sizeof eeprom_data; n += 4) {
            UARTprintf("+%02x = %08x\n", n, ((uint32_t*)&eeprom_data)[n>>2]);
        }
        return;
    }

    UARTprintf("Commands:\n"
               "  setwinch <number>\n"
               "  setflyer\n"
               "  show\n"
               "  reboot\n"
               "  exit\n");
}

void console_loop(void)
{
    while (1) {
        // Wait for enter to activate the console
        UARTEchoSet(false);
        while (1) {
            char c = UARTgetc();
            if (c == '\n' || c == '\r') {
                break;
            }
        }
        UARTEchoSet(true);
        console_is_interactive = true;

        while (console_is_interactive) {
            // Command prompt
            UARTprintf("> ");
            char line[100];
            UARTgets(line, sizeof line - 1);
            line[sizeof line - 1] = '\0';
            console_command(line);
        }
    }
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
    uint32_t sysclock_hz = MAP_SysCtlClockFreqSet(SYSCTL_OSC_SETTING, 120000000);

    PinoutSet(true, false);

    UARTStdioConfig(0, 115200, sysclock_hz);
    UARTprintf("\n\n====\nTuco Flyer Firmware starting!\n\n");

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    MAP_EEPROMInit();
    MAP_EEPROMRead((uint32_t*) &eeprom_data, EEPROM_OFFSET, sizeof eeprom_data);

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

    lwIPInit(sysclock_hz, mac, eeprom_data.ip_addr, eeprom_data.ip_netmask, eeprom_data.ip_gateway, IPADDR_USE_STATIC);

    bot_udp_pcb = udp_new();
    udp_bind(bot_udp_pcb, IP_ADDR_ANY, BOT_UDP_PORT);
    udp_recv(bot_udp_pcb, udp_received, 0);

    console_loop();
}
