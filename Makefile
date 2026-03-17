#  _____     ___ ____
#   ____|   |    ____|      PSX2 OpenSource Project
#  |     ___|   |____       (C)2002, David Ryan
#							(C)2026, Jose l. Azagra
#  ------------------------------------------------------------------------

.PHONY: all clean install

all:
	$(MAKE) -C smap
	$(MAKE) -C smap-linux

clean:
	$(MAKE) -C smap clean
	$(MAKE) -C smap-linux clean
	$(RM) smap/ps2smap.irx smap/ps2smap.notiopmod.elf smap/ps2smap.notiopmod.stripped.elf
	$(RM) smap-linux/ps2smap.irx smap-linux/ps2smap.notiopmod.elf smap-linux/ps2smap.notiopmod.stripped.elf

install: all
	mkdir -p $(PS2DEV)/ps2eth/smap
	cp smap/ps2smap.irx $(PS2DEV)/ps2eth/smap/
	mkdir -p $(PS2SDK)/iop/irx/
	ln -sf $(PS2DEV)/ps2eth/smap/ps2smap.irx $(PS2SDK)/iop/irx/
