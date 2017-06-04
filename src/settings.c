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
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "drivers/pinout.h"
#include "settings.h"

struct settings_t settings;
uint8_t mac_address[8];
volatile bool console_is_interactive = false;
#define EEPROM_OFFSET 0


static void reboot(void)
{
    HWREG(NVIC_APINT) = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}

static void complete_setup(void)
{
    MAP_EEPROMProgram((uint32_t*) &settings, EEPROM_OFFSET, sizeof settings);
    reboot();
}

static void setup_flyer(void)
{
    UARTprintf("Setting up flyer\n");
    settings.ip_controller = 0x0a200001;
    settings.ip_addr = 0x0a200008;
    settings.ip_netmask = 0xffffff00;
    settings.ip_gateway = 0;
    settings.bot_options = BOT_HAS_GIMBAL;
    complete_setup();
}

static void setup_winch(int number)
{
    UARTprintf("Setting up winch #%d\n", number);
    settings.ip_controller = 0x0a200001;
    settings.ip_addr = 0x0a20000a + number;
    settings.ip_netmask = 0xffffff00;
    settings.ip_gateway = 0;
    settings.bot_options = BOT_HAS_WINCH;
    complete_setup();
}

static void console_command(char *line)
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
        for (unsigned n = 0; n < sizeof settings; n += 4) {
            UARTprintf("+%02x = %08x\n", n, ((uint32_t*)&settings)[n>>2]);
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

void Settings_Init()
{
    // Unpack and check stored MAC address
    uint32_t id0, id1;
    MAP_FlashUserGet(&id0, &id1);
    if (id0 == 0xffffffff || id1 == 0xffffffff) {
        UARTprintf("ERROR: Missing factory-programmed MAC address\n");
        while (1);
    }
    mac_address[0] = (id0 >>  0) & 0xff;
    mac_address[1] = (id0 >>  8) & 0xff;
    mac_address[2] = (id0 >> 16) & 0xff;
    mac_address[3] = (id1 >>  0) & 0xff;
    mac_address[4] = (id1 >>  8) & 0xff;
    mac_address[5] = (id1 >> 16) & 0xff;
    mac_address[6] = 0;
    mac_address[7] = 0;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    MAP_EEPROMInit();
    MAP_EEPROMRead((uint32_t*) &settings, EEPROM_OFFSET, sizeof settings);
}

void Settings_Write()
{
    MAP_EEPROMProgram((uint32_t*) &settings, EEPROM_OFFSET, sizeof settings);
}

void Settings_Console()
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
