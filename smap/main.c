/*
 * SMAP Driver for PS2
 * Copyright (c) Tord Lindstrom (pukko@home.se)
 * Copyright (c) adresd (adresd_ps2dev@yahoo.com)
 */

#include <stdio.h>
#include <sysclib.h>
#include <loadcore.h>
#include <thbase.h>
#include <thevent.h>
#include <thsemap.h>
#include <intrman.h>

#include <ps2ip.h>
#include "main.h"

#define dbgprintf(args...) DEBUG_PRINTF(args)
IRX_ID("smap_driver", 3, 1);

// Interface name
#define IFNAME0 's'
#define IFNAME1 'm'

// Type aliases
typedef struct ip4_addr IPAddr;
typedef struct netif NetIF;
typedef struct SMapIF SMapIF;
typedef struct pbuf PBuf;

// Error codes
#define ERR_OK   0   // No error
#define ERR_MEM -1   // Out of memory
#define ERR_CONN -6  // Not connected
#define ERR_IF -11   // Low-level netif error

#if USE_GP_REGISTER
extern void *_gp;
#endif

// --- Global variables ---
static struct pbuf *TxHead = NULL;
static struct pbuf *TxTail = NULL;
static NetIF NIF;

// --- Forward declarations ---
static void EnQTxPacket(struct pbuf *tx);
static err_t SMapIFInit(NetIF* pNetIF);
static inline int SMapInit(IPAddr *IP, IPAddr *NM, IPAddr *GW, int argc, char *argv[]);
static void PrintIP(const struct ip4_addr *pAddr);

// -----------------------------------------------------------------------------
// TX Queue Management
// -----------------------------------------------------------------------------
static void EnQTxPacket(struct pbuf *tx)
{
    int OldState;
    CpuSuspendIntr(&OldState);

    if (TxHead != NULL)
        TxHead->next = tx;

    TxHead = tx;
    tx->next = NULL;

    if (TxTail == NULL) // Queue empty
        TxTail = TxHead;

    CpuResumeIntr(OldState);
}

int SMapTxPacketNext(void **payload)
{
    int len = 0;
    if (TxTail != NULL) {
        *payload = TxTail->payload;
        len = TxTail->len;
    }
    return len;
}

void SMapTxPacketDeQ(void)
{
    struct pbuf *toFree = NULL;
    int OldState;

    CpuSuspendIntr(&OldState);
    if (TxTail != NULL) {
        toFree = TxTail;

        if (TxTail == TxHead) { // Last packet
            TxTail = NULL;
            TxHead = NULL;
        } else {
            TxTail = TxTail->next;
        }
    }
    CpuResumeIntr(OldState);

    if (toFree != NULL) {
        toFree->next = NULL;
        pbuf_free(toFree);
    }
}

// -----------------------------------------------------------------------------
// Low-level Output
// -----------------------------------------------------------------------------
static err_t SMapLowLevelOutput(NetIF* pNetIF, PBuf* pOutput)
{
    err_t result = ERR_OK;
    struct pbuf* pbuf;

#if USE_GP_REGISTER
    SaveGP();
#endif

    if (pOutput->tot_len > pOutput->len) {
        // Multiple pbufs, coalesce
        pbuf_ref(pOutput); // LWIP will free later
        if ((pbuf = pbuf_coalesce(pOutput, PBUF_RAW)) != pOutput) {
            EnQTxPacket(pbuf);
            SMAPXmit();
        } else
            result = ERR_MEM;
    } else {
        pbuf_ref(pOutput);
        EnQTxPacket(pOutput);
        SMAPXmit();
    }

#if USE_GP_REGISTER
    RestoreGP();
#endif

    return result;
}

// -----------------------------------------------------------------------------
// IP Output for older LWIP versions
// -----------------------------------------------------------------------------
#ifdef PRE_LWIP_130_COMPAT
static err_t SMapOutput(NetIF* pNetIF, PBuf* pOutput, IPAddr* pIPAddr)
{
    err_t result;
    PBuf* pBuf;

#if USE_GP_REGISTER
    SaveGP();
#endif

    pBuf = etharp_output(pNetIF, pIPAddr, pOutput);
    result = pBuf != NULL ? SMapLowLevelOutput(pNetIF, pBuf) : ERR_OK;

#if USE_GP_REGISTER
    RestoreGP();
#endif

    return result;
}
#endif

// -----------------------------------------------------------------------------
// Network Interface Initialization
// -----------------------------------------------------------------------------
static err_t SMapIFInit(NetIF* pNetIF)
{
#if USE_GP_REGISTER
    SaveGP();
#endif

    TxHead = NULL;
    TxTail = NULL;

    // Interface name
    pNetIF->name[0] = IFNAME0;
    pNetIF->name[1] = IFNAME1;

#ifdef PRE_LWIP_130_COMPAT
    pNetIF->output = &SMapOutput;
    pNetIF->flags |= (NETIF_FLAG_LINK_UP | NETIF_FLAG_BROADCAST);
#else
    pNetIF->output = &etharp_output;
    pNetIF->flags |= (NETIF_FLAG_ETHARP | NETIF_FLAG_BROADCAST);
#endif

    pNetIF->linkoutput = &SMapLowLevelOutput;
    pNetIF->hwaddr_len = NETIF_MAX_HWADDR_LEN;
    pNetIF->mtu = 1500;

    // Get MAC address
    SMAPGetMACAddress(pNetIF->hwaddr);
    dbgprintf("MAC address : %02x:%02x:%02x:%02x:%02x:%02x\n",
        pNetIF->hwaddr[0], pNetIF->hwaddr[1], pNetIF->hwaddr[2],
        pNetIF->hwaddr[3], pNetIF->hwaddr[4], pNetIF->hwaddr[5]);

    // Enable sending and receiving
    SMAPInitStart();

#if USE_GP_REGISTER
    RestoreGP();
#endif

    return ERR_OK;
}

// -----------------------------------------------------------------------------
// Packet Input (from interrupt context)
// -----------------------------------------------------------------------------
void SMapLowLevelInput(PBuf* pBuf)
{
    // Pass received data to ps2ip
    ps2ip_input(pBuf, &NIF);
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------
static inline int SMapInit(IPAddr *IP, IPAddr *NM, IPAddr *GW, int argc, char *argv[])
{
    if (smap_init(argc, argv) != 0)
        return 0;

    dbgprintf("SMapInit: SMap initialized\n");

    netif_add(&NIF, IP, NM, GW, &NIF, &SMapIFInit, tcpip_input);
    netif_set_default(&NIF);
    netif_set_up(&NIF);

    dbgprintf("SMapInit: NetIF added to ps2ip\n");
    return 1;
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------
static void PrintIP(const struct ip4_addr *pAddr)
{
    printf("%d.%d.%d.%d",
        (u8)pAddr->addr,
        (u8)(pAddr->addr >> 8),
        (u8)(pAddr->addr >> 16),
        (u8)(pAddr->addr >> 24));
}

// -----------------------------------------------------------------------------
// Link state callbacks
// -----------------------------------------------------------------------------
void PS2IPLinkStateUp(void)
{
    tcpip_callback((void*)&netif_set_link_up, &NIF);
}

void PS2IPLinkStateDown(void)
{
    tcpip_callback((void*)&netif_set_link_down, &NIF);
}

// -----------------------------------------------------------------------------
// Main entry
// -----------------------------------------------------------------------------
int _start(int argc, char *argv[])
{
    IPAddr IP, NM, GW;
    int numArgs;
    char **pArgv;

    DisplayBanner();

    // Parse IP arguments
    dbgprintf("SMAP: argc %d\n", argc);

    if (argc >= 4) {
        dbgprintf("SMAP: %s %s %s\n", argv[1], argv[2], argv[3]);
        IP.addr = inet_addr(argv[1]);
        NM.addr = inet_addr(argv[2]);
        GW.addr = inet_addr(argv[3]);

        numArgs = argc - 4;
        pArgv = &argv[4];
    } else {
        IP4_ADDR(&IP, 192, 168, 1, 100);
        IP4_ADDR(&NM, 255, 255, 255, 0);
        IP4_ADDR(&GW, 192, 168, 1, 1);

        numArgs = argc - 1;
        pArgv = &argv[1];
    }

    if (!SMapInit(&IP, &NM, &GW, numArgs, pArgv))
        return MODULE_NO_RESIDENT_END;

    printf("IP Address: ");
    PrintIP(&IP);
    printf(", NetMask: ");
    PrintIP(&NM);
    printf(", Gateway: ");
    PrintIP(&GW);
    printf("\n");

    return MODULE_RESIDENT_END;
}