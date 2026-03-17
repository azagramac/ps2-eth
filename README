# PS2ETH

This project is dedicated to developing **PlayStation 2 Ethernet drivers**.  
Each driver may be covered by a different software license. Please check the license of the driver before using it.

---

## Current Drivers

### smap
- Driver for the **standard Sony Ethernet adapter**.  
- Based on `smap.irx` from retail PS2 games.  
- Supports **DMA transfers**.  
- Used as a **loadable `.IRX` module** by `.ELF` applications on the PS2.  
- License: check individual source files.

### smap-linux
- Driver for the **standard Sony Ethernet adapter** used by **PS2 Linux**.  
- Compatible API with `smap`, licensed under **GPL**.  
- Integrates into the Linux kernel running on the IOP for networking support.

---

## Driver Usage Clarification

There are two main PS2 Ethernet drivers with different use cases:

### smap
- The **official PS2 retail driver**.  
- Primarily used as a **loadable `.IRX` module** by any `.ELF` application on the PS2.  
- Provides:
  - EMAC and DMA initialization  
  - TX/RX interrupt handling  
  - A simple API for sending and receiving packets from the EE  
- **Use case:** Homebrew games or applications can load `ps2smap.irx` to send/receive data using their own networking stack (e.g., lwIP).

### smap-linux
- **Adapted for PS2 Linux**.  
- Integrates into the **Linux kernel running on the IOP** as part of the Linux networking stack.  
- Uses the same base as `smap` but modified for Linux API compatibility and licensed under **GPL**.  
- **Use case:** Allows PS2 Linux to detect the network interface automatically, enabling commands like `ifconfig` or `ping`.

| Driver         | Primary Use                                  | Load Method       | License                     |
|----------------|---------------------------------------------|-----------------|----------------------------|
| `smap`         | PS2 retail / homebrew `.ELF` applications  | Manual `.IRX`   | Sony / Dan Potter (varies) |
| `smap-linux`   | PS2 Linux kernel / networking               | Automatic       | GPL                        |

---

## Technical Overview of PS2 Ethernet Drivers

The PlayStation 2 includes a **Sony Ethernet Adapter**:

- **PS2 Fat**: compatible with SCPH-30000 to SCPH-50000 series (SCPH-10350 Ethernet Adapter)  

The adapter supports **10/100 Mbps** and **full/half duplex** via an MII interface.

### Architecture

- **EE (Emotion Engine)**: main CPU, runs the game/application.  
- **IOP (I/O Processor)**: auxiliary CPU, runs IRX drivers and handles Ethernet.  

Drivers execute on the **IOP**, exposing APIs to the EE through **SIF RPC** or direct calls.

### DMA and Descriptors

- Uses **circular TX/RX descriptors** pointing to data buffers.  
- Registers used include: `EMAC3`, `RXEND`, `TXEND`, `RXDNV`, `TXDNV`.  
- Handles interrupts to signal packet transmission, reception, or errors.

### Interrupts

- `INTR_EMAC3`: EMAC3 interrupt  
- `INTR_RXEND`: packet received  
- `INTR_TXEND`: packet transmitted  
- `INTR_RXDNV` / `INTR_TXDNV`: invalid descriptor error

### API Functions

- **Initialization / control**:  
  `SMap_Init()`, `SMap_Start()`, `SMap_Stop()`  
- **Transmission / reception**:  
  `SMap_CanSend()`, `SMap_Send(struct pbuf* pPacket)`  
  `SMap_HandleTXInterrupt(int iFlags)`, `SMap_HandleRXEMACInterrupt(int iFlags)`  
- **Configuration / status**:  
  `SMap_GetMACAddress()`, `SMap_EnableInterrupts(int iFlags)`  
  `SMap_DisableInterrupts(int iFlags)`, `SMap_ClearIRQ(int iFlags)`

### Buffers and Memory

- Usually uses **lwIP-style pbufs** (`struct pbuf`).  
- DMA buffers must reside in **IOP-accessible memory**, 16-byte aligned.  
- Supports **zero-copy transfers** for large packets.

### Compatibility and Licensing

- **smap**: original Sony / Dan Potter code, license may vary.  
- **smap-linux**: GPL, used in PS2 Linux official kit.  
- Both expose a similar API, allowing interoperability between retail PS2 games and PS2 Linux.

---

## Build driver instructions

```bash
make all
make install
```

## Generated Files, located in smap and smap-linux folders:

- ps2smap.irx
- ps2smap.notiopmod.elf
- ps2smap.notiopmod.stripped.elf

## Clean generated binaries

```bash
make clean
```