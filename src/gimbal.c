#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_uart.h"
#include "inc/hw_nvic.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "utils/ringbuf.h"
#include "utils/lwiplib.h"
#include "drivers/pinout.h"
#include "settings.h"
#include "gimbal.h"
#include "botproto.h"
#include "protocol_defs.h"

// UART RX on PC4, UART TX on PC5
#define GIMBAL_BAUD             115200
#define GIMBAL_UART_BASE        UART7_BASE
#define GIMBAL_UART_INT         INT_UART7
#define GIMBAL_UART_PERIPH      SYSCTL_PERIPH_UART7

static bool gimbal_initialized = false;
static bool gimbal_rx_flush = false;
static tRingBufObject gimbal_tx_ring;
static tRingBufObject gimbal_rx_ring;
static uint8_t gimbal_tx_mem[2048];
static uint8_t gimbal_rx_mem[512];


void Gimbal_Init(uint32_t sysclock_hz)
{
    gimbal_initialized = true;
    RingBufInit(&gimbal_tx_ring, gimbal_tx_mem, sizeof gimbal_tx_mem);
    RingBufInit(&gimbal_rx_ring, gimbal_rx_mem, sizeof gimbal_rx_mem);
    MAP_SysCtlPeripheralEnable(GIMBAL_UART_PERIPH);
    MAP_UARTConfigSetExpClk(GIMBAL_UART_BASE, sysclock_hz, GIMBAL_BAUD, UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE | UART_CONFIG_WLEN_8);
    MAP_UARTFIFOLevelSet(GIMBAL_UART_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    MAP_UARTIntEnable(GIMBAL_UART_BASE, UART_INT_RX | UART_INT_RT);
    MAP_IntEnable(GIMBAL_UART_INT);
    MAP_GPIOPinConfigure(GPIO_PC4_U7RX);
    MAP_GPIOPinConfigure(GPIO_PC5_U7TX);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
}

void Gimbal_TxQueue(struct pbuf *p)
{
    // Called from lwIP context, with incoming data to transmit asynchronously
    if (!gimbal_initialized) {
        return;
    }

    // Write as much data as will fit; ignore any overflow
    while (p && p->len && p->len < RingBufFree(&gimbal_tx_ring)) {
        RingBufWrite(&gimbal_tx_ring, (uint8_t*) p->payload, p->len);
        p = p->next;
    }

    Gimbal_Poll();
}

static void gimbal_continue_tx(void)
{
    // Move some data from our TX buffer to the UART.
    // Called in UART ISR or with UART ISR disabled.

    MAP_UARTIntDisable(GIMBAL_UART_BASE, UART_INT_TX);

    while (MAP_UARTSpaceAvail(GIMBAL_UART_BASE) && !RingBufEmpty(&gimbal_tx_ring)) {
        MAP_UARTCharPutNonBlocking(GIMBAL_UART_BASE, RingBufReadOne(&gimbal_tx_ring));
    }

    if (!RingBufEmpty(&gimbal_tx_ring)) {
        MAP_UARTIntEnable(GIMBAL_UART_BASE, UART_INT_TX);
    }
}

void Gimbal_Poll(void)
{
    // Called from lwIP context, to pump data through the receive and transmit pipes.
    // If we see any complete gimbal packets sitting in the buffer, we immediately flush
    // them out to the controller in a single UDP packet.

    if (!gimbal_initialized) {
        return;
    }

    // If we're flushing, move a chunk of data from the RX buffer to lwIP
    if (gimbal_rx_flush) {
        gimbal_rx_flush = false;
        uint32_t flush_len = RingBufUsed(&gimbal_rx_ring);
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, 1 + flush_len, PBUF_RAM);
        uint8_t *data = (uint8_t*) p->payload;
        data[0] = BOT_MSG_GIMBAL;
        RingBufRead(&gimbal_rx_ring, data+1, flush_len);
        BotProto_Send(p);
        pbuf_free(p);
    }

    // If we have data to transmit, get it started here and continue in the ISR
    if (!RingBufEmpty(&gimbal_tx_ring)) {
        MAP_IntDisable(GIMBAL_UART_INT);
        gimbal_continue_tx();
        MAP_IntEnable(GIMBAL_UART_INT);
    }
}

static bool gimbal_packet_detect(uint8_t c)
{
    // Simple packet detector FSM. Supports both long (bootloader) and short (normal) formats.
    // 55 AA <target> <cmd> <len16> <data> <crc16>
    // A5 5A <target> <cmd> <len8> <data> <crc16>

    static uint8_t state = 0;
    static uint16_t length = 0;
    const uint16_t max_len = 0x500;

    switch (state) {

        case 0: // Idle
            if (c == 0x55) {
                state = 1;
            } else if (c == 0xA5) {
                state = 2;
            }
            break;

        case 1: // 55
            if (c == 0xAA) {
                state = 3;
            } else if (c == 0x55) {
                state = 1;
            } else if (c == 0xA5) {
                state = 2;
            } else {
                state = 0;
            }
            break;

        case 2: // A5
            if (c == 0x5A) {
                state = 7;
            } else if (c == 0x55) {
                state = 1;
            } else if (c == 0xA5) {
                state = 2;
            } else {
                state = 0;
            }
            break;

        case 3: // 55 AA +target
        case 4: // 55 AA target +cmd
            state++;
            break;

        case 5: // 55 AA target cmd +lenL
            length = c;
            state++;
            break;

        case 6: // 55 AA target cmd lenL +lenH
            length |= c << 8;
            state = length < max_len ? 10 : 0;
            break;

        case 7: // A5 5A +target
        case 8: // A5 5A target +cmd
            state++;
            break;

        case 9: // A5 5A target cmd +len
            length = c;
            state++;
            break;

        case 10: // Wait for payload + first CRC byte
            if (length) {
                length--;
            } else {
                state++;
            }
            break;

        case 11: // Last CRC byte
            state = 0;
            return true;
    }

    return false;
}

static void gimbal_rx_char(uint8_t c)
{
    if (!RingBufFull(&gimbal_rx_ring)) {
        RingBufWriteOne(&gimbal_rx_ring, c);
    }

    // In parallel, run a state machine that looks for packets, and if we see one trigger a flush.
    if (gimbal_packet_detect(c)) {
        gimbal_rx_flush = true;
    }

    // Also flush if the buffer is more than half full
    if (RingBufUsed(&gimbal_rx_ring) > sizeof gimbal_rx_mem / 2) {
        gimbal_rx_flush = true;
    }
}

void Gimbal_UartIrq(void)
{
    uint32_t status = MAP_UARTIntStatus(GIMBAL_UART_BASE, true);
    MAP_UARTIntClear(GIMBAL_UART_BASE, status);

    // Store incoming data in our own lock-free queue
    if (status & (UART_INT_RX | UART_INT_RT)) {
        while (MAP_UARTCharsAvail(GIMBAL_UART_BASE)) {
            gimbal_rx_char(MAP_UARTCharGetNonBlocking(GIMBAL_UART_BASE));
        }
    }

    // Make transmit progress
    if (status & UART_INT_TX) {
        gimbal_continue_tx();
    }

    // If the rx queue needs flushing, wake up Gimbal_Poll()
    if (gimbal_rx_flush) {
        HWREG(NVIC_SW_TRIG) |= INT_EMAC0 - 16;
    }
}
