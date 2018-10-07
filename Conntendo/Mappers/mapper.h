#pragma once
//----------------------------------------------------------------//
// Emulates NES Mappers to play multiple types of NES ROMs
// Base Mapper File
//----------------------------------------------------------------//

#include <cstring>
#include "common.h"

namespace MAPPER
{
	// Mapper Save Data
	struct SaveData
	{
		u8 mapperData[64]; // All Purpose ( additional registers, flags etc )
		u8 prgRAM[K_8]; 
		u8 chrRAM[K_8];

		u32 prgMap[4];
		u32 chrMap[8];

		// Only for MMC3
		u8 extraRAM[K_4];

		// Default Constructor
		SaveData() 
		{ 
			memset(mapperData, 0, sizeof(mapperData)); 
		};

		SaveData( u8* nPrgRam, u8* nChrRam, bool hasChrRam, u32 nPrgMap[], u32 nChrMap[] )
		{		
			memset(mapperData, 0, sizeof(mapperData));

			if (hasChrRam)
			{
				for (int i = 0; i < K_8; i++)
				{
					chrRAM[i] = nChrRam[i];
				} // for
			}

			for (int i = 0; i < K_8; i++)
			{
				prgRAM[i] = nPrgRam[i];
			} // for	

			for (int i = 0; i < 4; i++)
			{
				prgMap[i] = nPrgMap[i];
			} // for

			for (int i = 0; i < 8; i++)
			{
				chrMap[i] = nChrMap[i];
			} // for
		}

	}; // SaveData

} // MAPPER

// Base Mapper Class for extendable cartridge chipsets
class Mapper
{
public:

	Mapper(u8* rom);
	~Mapper();

	// Read-Write Functions
	virtual u8 read8(u16 address);
	virtual u8 write8( u16 address, u8 val ) { return val; }
	virtual u8 chr_read8(u16 address);
	virtual u8 chr_write8( u16 address, u8 val ) { return val; }

	// IRQ Messaging between Mapper and System
	virtual void SignalScanline() {} // for Scanline Counter IRQ (MMC3)
	virtual void SignalCPU() {} // for CPU-Counter IRQ Systems (VRC4, FME-7)

	// Extra vRAM on some rare cartridges ( Gauntlet )
	virtual u8 ReadExtraRAM(u16 address) { return 0;  }
	virtual u8 WriteExtraRAM(u16 address, u8 val) { return 0; }

	// SaveStates
	virtual MAPPER::SaveData GrabSaveData();
	virtual void LoadSaveData(MAPPER::SaveData loadedData);

protected:

	// PRG and CHR Address Map
	u32 prgMap[4]; // Four  8K Slots
	u32 chrMap[8]; // Eight 1K Slots

	// Emulated Memory Pointers
	u8* rom;
	u8* prg;
	u8* chr;
	u8* prgRAM;

	// Bank Sizes read from ROM
	u32 prgSize;
	u32 chrSize;
	u32 prgRAMSize;

	// Cartridge Flags
	bool isLargeROM = false; // for specific mappers to determine if special-case
	bool hasChrRAM  = false; // whether or not cartridge contains Chr RAM

	// Memory Remapping 
	void MapPRG(int pageSize, int slot, int bank);
	void MapCHR(int pageSize, int slot, int bank);

}; //Mapper