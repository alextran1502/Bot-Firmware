#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inc/hw_ssi.h"
#include "inc/hw_udma.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "utils/lwiplib.h"
#include "settings.h"
#include "leds.h"

#define LED_SSI_BASE        SSI2_BASE
#define LED_CLOCK_HZ        2500000
#define LED_SSI_PERIPH      SYSCTL_PERIPH_SSI2
#define LED_UDMA            UDMA_CH13_SSI2TX
#define LED_MAX_PIXELS      200
#define LED_PIXEL_OFF       htonl(0xE0000000)

static struct {
    uint32_t start[1];
    uint32_t pixels[LED_MAX_PIXELS];
    uint32_t end[(LED_MAX_PIXELS + 31) / 32];
} led_buffer;

static void led_async_write(void)
{
    void *ssi_dr = (void*) (LED_SSI_BASE + SSI_O_DR);
    MAP_uDMAChannelTransferSet(LED_UDMA, UDMA_MODE_BASIC, &led_buffer, ssi_dr, sizeof led_buffer);
    MAP_uDMAChannelEnable(LED_UDMA);
}

void LEDs_Init(uint32_t sysclock_hz)
{
    memset(&led_buffer.start, 0x00, sizeof led_buffer.start);
    memset(&led_buffer.end, 0xFF, sizeof led_buffer.end);
    for (unsigned i = 0; i < LED_MAX_PIXELS; i++) {
        led_buffer.pixels[i] = LED_PIXEL_OFF;
    }

    MAP_SysCtlPeripheralEnable(LED_SSI_PERIPH);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    MAP_GPIOPinConfigure(GPIO_PD3_SSI2CLK);
    MAP_GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
    MAP_SSIConfigSetExpClk(LED_SSI_BASE, sysclock_hz, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER, LED_CLOCK_HZ, 8);
    MAP_SSIEnable(LED_SSI_BASE);
    MAP_GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_3 | GPIO_PIN_1);
    MAP_uDMAChannelAssign(LED_UDMA);
    MAP_uDMAChannelAttributeEnable(LED_UDMA, UDMA_ATTR_USEBURST);
    MAP_uDMAChannelControlSet(LED_UDMA, UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_4);
    MAP_SSIDMAEnable(LED_SSI_BASE, SSI_DMA_TX);

    // The APA102s can power on with arbitrary colors set; refresh them to all-off right away.
    led_async_write();
}

static void copy_packet_to_framebuffer(struct pbuf *p)
{
    uint32_t *pixels = led_buffer.pixels;
    uint8_t *pixel_bytes = (uint8_t*) pixels;
    uint32_t byte_offset = 0;
    const uint32_t byte_limit = sizeof led_buffer.pixels;

    // Reconstitute a linked list of pbufs in linear form, in the framebuffer.
    // Realistically all of these packets should be in a single pbuf, but completeness!
    while (p && byte_offset < byte_limit) {
        uint8_t *data = (uint8_t*) p->payload;
        unsigned len = p->len;
        while (len && byte_offset < byte_limit) {
            pixel_bytes[byte_offset] = *data;
            byte_offset++;
            data++;
            len--;
        }
        p = p->next;
    }

    // Overwrite the remainder of the framebuffer, starting after the last complete pixel
    // written, in order to clear any previous (larger) frames, as well as to avoid sending
    // padding bytes or incomplete pixels to the hardware.

    uint32_t pixel_offset = byte_offset / sizeof led_buffer.pixels[0];
    while (pixel_offset < LED_MAX_PIXELS) {
        pixels[pixel_offset] = LED_PIXEL_OFF;
        pixel_offset++;
    }
}

void LEDs_Command(struct pbuf *p)
{
    copy_packet_to_framebuffer(p);
    led_async_write();
}
