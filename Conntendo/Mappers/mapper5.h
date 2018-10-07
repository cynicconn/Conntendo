#pragma once
#include "mapper.h"

// MMC5 (The Ultimate Mapper)
class Mapper5 : public Mapper
{
public:

	// Setup
	Mapper5(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 read8(u16 address);
	u8 write8(u16 address, u8 val);
	u8 chr_read8(u16 address);
	u8 chr_write8(u16 address, u8 val);

	// This Mapper reads NameTables different then all others
	u8 ReadExtraRAM(u16 address);
	u8 WriteExtraRAM(u16 address, u8 val);

	// IRQ
	void SignalScanline();
	void SignalCPU(); // for checking if stop rendering

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:

	// Helper Function
	u8 ReadAudio(u16 address);
	u8 WriteAudio(u16 address, u8 val);
	u8 WriteToRAM(u16 address, u8 val);
	void Mapper5::MapChrBG(int pageSize, int slot, int bank);

	u8 prgBankMode; // To determine one of four PRG Banking Modes 
	u8 chrBankMode; // To determine one of four CHR Banking Modes (All (except one) games use 1KB pages)

	// NameTable
	u8 nameTableMapping;
	u8 fillModeTile;
	u8 fillModeColor;

	// Split Mode TODO
	u8 verticalSplitMode;
	u8 verticalSplitScroll;
	u8 verticalSplitBank;

	// IRQ
	u8 scanlineCounter;
	u8 triggerScanline;
	bool irqEnable;
	bool irqPending;
	bool inFrame; // if true, PPU is currently rendering a scanline

	// PRG and CHR Banks
	u8 prgBank[4];
	u8 sprChrBank[8];
	u8 bgChrBank[4];	// Background Tiles (Four Mirrored)
	u32 bgChrMap[8];	// Eight 1K Slots (Four Mirrored)
	bool isBankROM[3];	// not currently used (not in SaveData)
	u8 upperChrBankBits;

	// Multiply Instruction (Wow, multiplying on 6502!)
	u8 multiplicand;
	u8 multiplier;

	// Custom PRG RAM
	u8 lgPrgRAM[K_64];

	// Custom MMC5 NameTables
	u8 ntLowerRAM[K_1];
	u8 ntUpperRAM[K_1];

	// 1K On-Chip Extra Memory
	u8 extraRAM[K_1];
	u8 ramExtraMode;
	u8 ramBankSwitch;
	u8 extraVal;
	u8 attrModeBank;
	bool ramProtect1;
	bool ramProtect2;
};
