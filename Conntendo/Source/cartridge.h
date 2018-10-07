#pragma once
//----------------------------------------------------------------//
// The virtual "cartridge" that the emulator reads and writes to
//----------------------------------------------------------------//

// Conntendo
#include "common.h"
#include "mapper.h"

namespace Cartridge 
{
	// ROM Grabbing Functions
	string GetGameName();
	bool LoadROM(const char* romName);

	// Game Savestates ( currently only one per ROM supported )
	bool CreateSaveState(int slot);
	bool LoadSaveState(int slot);

	// ROM Read-Write Functions
	u8 WriteCHR(u16 address, u8 val);
	u8 ReadCHR(u16 address);
	u8 WritePRG(u16 address, u8 val);
	u8 ReadPRG(u16 address);

	// Mapper Functions
	void SignalScanline();
	void SignalCPU();
	Mapper* GetMapper();

	// Extra Cartridge vRAM ( Gauntlet, Rad Racer II etc )
	u8 ReadExtraRAM(u16 address);
	u8 WriteExtraRAM(u16 address, u8 val);

} // Cartridge
