/*
 * SMAP Driver Support Structures and Macros
 * Original calls to DEBUG_PRINTF() were to sceInetPrintf().
 */
#define DEBUG_PRINTF(args...) printf(args)

#if USE_GP_REGISTER
/*
 * Save and restore the GP register.
 * The syntax here is specific to PS2 GCC and IOP kernel.
 * _ori_gp is early-clobbered to save GP before modifications.
 */
#define SaveGP() \
    void *_ori_gp; \
    __asm volatile("move %0, $gp\n" \
                   "move $gp, %1" : "=&r"(_ori_gp) : "r"(&_gp) : "gp")

#define RestoreGP() \
    __asm volatile("move $gp, %0" :: "r"(_ori_gp) : "gp")
#endif

// -----------------------------------------------------------------------------
// Runtime statistics for SMAP driver
// -----------------------------------------------------------------------------
struct RuntimeStats {
    u32 RxDroppedFrameCount;
    u32 RxErrorCount;
    u16 RxFrameOverrunCount;
    u16 RxFrameBadLengthCount;
    u16 RxFrameBadFCSCount;
    u16 RxFrameBadAlignmentCount;
    u32 TxDroppedFrameCount;
    u32 TxErrorCount;
    u16 TxFrameLOSSCRCount;
    u16 TxFrameEDEFERCount;
    u16 TxFrameCollisionCount;
    u16 TxFrameUnderrunCount;
    u16 RxAllocFail;
};

// -----------------------------------------------------------------------------
// SMAP Driver internal data
// -----------------------------------------------------------------------------
struct SmapDriverData {
    volatile u8 *smap_regbase;       	// Base address for SMAP registers
    volatile u8 *emac3_regbase;      	// Base address for EMAC3
    unsigned int TxBufferSpaceAvailable;
    unsigned char NumPacketsInTx;
    unsigned char TxBDIndex;
    unsigned char TxDNVBDIndex;
    unsigned char RxBDIndex;
    void *packetToSend;
    int Dev9IntrEventFlag;
    int IntrHandlerThreadID;
    unsigned char SmapDriverStarted;     // SMAP driver has started
    unsigned char SmapIsInitialized;     // Driver initialized (software)
    unsigned char NetDevStopFlag;
    unsigned char EnableLinkCheckTimer;
    unsigned char LinkStatus;             // Hardware Ethernet link status
    unsigned char LinkMode;
    iop_sys_clock_t LinkCheckTimer;
    struct RuntimeStats RuntimeStats;     // Driver statistics
};

// -----------------------------------------------------------------------------
// Event flag bits
// -----------------------------------------------------------------------------
#define SMAP_EVENT_START       0x01
#define SMAP_EVENT_STOP        0x02
#define SMAP_EVENT_INTR        0x04
#define SMAP_EVENT_XMIT        0x08
#define SMAP_EVENT_LINK_CHECK  0x10

// -----------------------------------------------------------------------------
// Public function prototypes
// -----------------------------------------------------------------------------
int DisplayBanner(void);
int smap_init(int argc, char *argv[]);
int SMAPInitStart(void);
int SMAPStart(void);

void SMAPStop(void);
void SMAPXmit(void);

int SMAPGetMACAddress(u8 *buffer);

void PS2IPLinkStateUp(void);
void PS2IPLinkStateDown(void);
void SMapLowLevelInput(struct pbuf* pBuf);

int SMapTxPacketNext(void **payload);

void SMapTxPacketDeQ(void);

// -----------------------------------------------------------------------------
// SMAP data transfer include
// -----------------------------------------------------------------------------
#include "xfer.h"