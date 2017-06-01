#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_emac.h"
#include "driverlib/gpio.h"
#include "driverlib/emac.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "drivers/pinout.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"

void init_dma_frames(void);
void poll_rx(void);

typedef struct {
    tEMACDMADescriptor desc;
    uint8_t frame[1536];
} tDMAFrame;

uint32_t g_ui32SysClock;

tDMAFrame g_rxBuffer[8];
tDMAFrame g_txBuffer[8];
uint8_t g_nextRx = 0;


int main(void)
{
    ROM_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);

    PinoutSet(true, false);
    ROM_IntMasterEnable();
    UARTStdioConfig(0, 115200, g_ui32SysClock);

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0));

    MAP_EMACPHYConfigSet(EMAC0_BASE,
                         EMAC_PHY_TYPE_INTERNAL |
                         EMAC_PHY_INT_MDIX_EN |
                         EMAC_PHY_AN_100B_T_FULL_DUPLEX);

    ROM_EMACReset(EMAC0_BASE);

    ROM_EMACInit(EMAC0_BASE, g_ui32SysClock,
                 EMAC_BCONFIG_MIXED_BURST | EMAC_BCONFIG_PRIORITY_FIXED,
                 8, 8, 0);

    ROM_EMACConfigSet(EMAC0_BASE,
                      (EMAC_CONFIG_FULL_DUPLEX |
                       EMAC_CONFIG_7BYTE_PREAMBLE |
                       EMAC_CONFIG_IF_GAP_96BITS |
                       EMAC_CONFIG_USE_MACADDR0 |
                       EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                       EMAC_CONFIG_BO_LIMIT_1024),
                      (EMAC_MODE_RX_STORE_FORWARD |
                       EMAC_MODE_TX_STORE_FORWARD ), 0);

    ROM_EMACFrameFilterSet(EMAC0_BASE, EMAC_FRMFILTER_RX_ALL);

    init_dma_frames();

    ROM_EMACTxEnable(EMAC0_BASE);
    ROM_EMACRxEnable(EMAC0_BASE);

    while (1) {
        poll_rx();
    }
}

void poll_rx(void)
{        
    const uint32_t num_rx = sizeof(g_rxBuffer) / sizeof(g_rxBuffer[0]);
    uint32_t status = g_rxBuffer[g_nextRx].desc.ui32CtrlStatus;
    uint32_t framelen = ((g_rxBuffer[g_nextRx].desc.ui32CtrlStatus &
                            DES0_RX_STAT_FRAME_LENGTH_M) >> DES0_RX_STAT_FRAME_LENGTH_S);

    if (status & DES0_RX_CTRL_OWN) {
        // Hardware not done with this buffer yet
        return;
    }

    UARTprintf("Whoa, packet %d bytes\n", framelen);

    // Done with this buffer
    g_rxBuffer[g_nextRx].desc.ui32CtrlStatus = DES0_RX_CTRL_OWN;
    g_nextRx = (g_nextRx + 1) % num_rx;
    ROM_EMACRxDMAPollDemand(EMAC0_BASE);
}

void init_dma_frames(void)
{
    const uint32_t num_tx = sizeof(g_txBuffer) / sizeof(g_txBuffer[0]);
    const uint32_t num_rx = sizeof(g_rxBuffer) / sizeof(g_rxBuffer[0]);
    uint32_t i;

    for (i = 0; i < num_tx; i++) {
        g_txBuffer[i].desc.ui32Count = (sizeof g_txBuffer[0].frame << DES1_TX_CTRL_BUFF1_SIZE_S);
        g_txBuffer[i].desc.pvBuffer1 = g_txBuffer[i].frame;
        g_txBuffer[i].desc.DES3.pLink = &g_txBuffer[(i + 1) % num_tx].desc;
        g_txBuffer[i].desc.ui32CtrlStatus = 0;
    }

    for (i = 0; i < num_rx; i++) {
        g_rxBuffer[i].desc.ui32Count = DES1_RX_CTRL_CHAINED | (sizeof g_rxBuffer[0].frame << DES1_RX_CTRL_BUFF1_SIZE_S);
        g_rxBuffer[i].desc.pvBuffer1 = g_rxBuffer[i].frame;
        g_rxBuffer[i].desc.DES3.pLink = &g_rxBuffer[(i + 1) % num_rx].desc;
        g_rxBuffer[i].desc.ui32CtrlStatus = DES0_RX_CTRL_OWN;
    }

    ROM_EMACRxDMADescriptorListSet(EMAC0_BASE, &g_rxBuffer[0].desc);
    ROM_EMACTxDMADescriptorListSet(EMAC0_BASE, &g_txBuffer[0].desc);
}

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
    while(1)
    {
        // runtime error in driver lib
    }
}
#endif
