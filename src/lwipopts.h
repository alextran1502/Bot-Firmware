#pragma once

#define HOST_TMR_INTERVAL       100
#define EMAC_PHY_CONFIG         (EMAC_PHY_TYPE_INTERNAL | EMAC_PHY_INT_MDIX_EN | EMAC_PHY_AN_100B_T_FULL_DUPLEX)
#define PHY_PHYS_ADDR           0
#define NUM_TX_DESCRIPTORS      24
#define NUM_RX_DESCRIPTORS      8

#define SYS_LIGHTWEIGHT_PROT    1
#define NO_SYS                  1
#define MEM_ALIGNMENT           4
#define MEM_SIZE                (64 * 1024)
#define MEMP_NUM_PBUF           48
#define MEMP_NUM_TCP_PCB        16
#define MEMP_NUM_SYS_TIMEOUT    8
#define PBUF_POOL_SIZE          48
#define PBUF_LINK_HLEN          16
#define PBUF_POOL_BUFSIZE       512
#define ETH_PAD_SIZE            0


#define IP_REASSEMBLY           0
#define IP_FRAG                 0

#define LWIP_DHCP               0
#define LWIP_AUTOIP             0
#define LWIP_TCP                0
#define LWIP_NETCONN            0
#define LWIP_SOCKET             0

#define CHECKSUM_GEN_IP         0
#define CHECKSUM_GEN_ICMP       0
#define CHECKSUM_GEN_UDP        0
#define CHECKSUM_GEN_TCP        0
#define CHECKSUM_CHECK_IP       0
#define CHECKSUM_CHECK_UDP      0
#define CHECKSUM_CHECK_TCP      0

#define LWIP_DBG_MIN_LEVEL      LWIP_DBG_LEVEL_OFF
#define LWIP_DBG_TYPES_ON       (LWIP_DBG_ON | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_FRESH)
