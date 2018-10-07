#pragma once
#include "mapper.h"

// MMC3 for Super Mario 3, Kirbys Adventure etc
class Mapper4 : public Mapper
{
public:
	Mapper4(u8* rom);
	void SetBanks();

	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

	void SignalScanline();

	// Extra vRAM on some rare cartridges ( Gauntlet, Rad Racer 2 )
	u8 ReadExtraRAM(u16 address); 
	u8 WriteExtraRAM(u16 address, u8 val); 

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	// Control Registers
	u8 bankSelect; 
	u8 bankData[8];
	bool horMirroring;

	// IRQ Registers
	u8 irqLatch;
	u8 irqReload;
	bool irqDisable;
	bool irqEnable;

	// for 4K vRAM built into the cartridge
	u8 extraRAM[K_4];

};
