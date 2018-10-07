#include "ppu.h"
#include "mapper1.h"

#define RAM_ENABLED		!(prgBank & 0x10)
#define MIRROR_MODE		(control & 0x03)
#define USE_UPPER_BANK	(chrBank0 & 0x10)
#define PRG_BANK_MODE	((control & 0x0C) >> 2)

// Constructor
Mapper1::Mapper1(u8* rom) : Mapper(rom)
{
	isLargeROM = (prgSize > K_256); 

	control		= 0x0C;
	prgBank		= 0;
	chrBank0	= 0;
	chrBank1	= 0;

	shiftRegister	= 0;
	writeCount		= 0;

	SetBanks();
}

void Mapper1::SetBanks()
{
	u8 upper256K = (USE_UPPER_BANK && isLargeROM) ? 16 : 0; // for ROMs bigger than 256K

	// PRG Banking
	switch (PRG_BANK_MODE)
	{
	case 0: // 32KB PRG Mode
	case 1:
		MapPRG(32, 0, (prgBank & 0x0F) >> 1);
		break;
	case 2: // Fixed 0x8000, Swappable 0xC000
		MapPRG(16, 0, 0);
		MapPRG(16, 1, prgBank & 0x0F);
		break;
	case 3: // Fixed 0xC000, Swappable 0x8000
		MapPRG(16, 0, (prgBank & 0x0F) + upper256K);
		MapPRG(16, 1, 0x0F + upper256K);
		break;
	} // switch

	// CHR Banking
	bool is4KChrMode = (control & 0x10) && !isLargeROM;
	if (is4KChrMode) // 4KB CHR Mode
	{
		MapCHR(4,0, chrBank0);
		MapCHR(4,1, chrBank1);
	}
	else // 8KB CHR Mode
	{
		MapCHR(8, 0, chrBank0 >> 1); // bit0 is not used
	}

	// Set Mirroring
	switch (MIRROR_MODE)
	{
	case 0:
		SetMirrorMode(PPU::ONESCREEN_LOW);
		break;
	case 1:
		SetMirrorMode(PPU::ONESCREEN_UP);
		break;
	case 2: 
		SetMirrorMode(PPU::VERTICAL);
		break;
	case 3:  
		SetMirrorMode(PPU::HORIZONTAL);
		break;
	} // switch

} // SetBanks()

u8 Mapper1::read8(u16 address)
{
	if (address >= K_32) // Read PRG ROM
	{
		u16 offsetAddress = (address - K_32);
		u32 mapVal = prgMap[offsetAddress / K_8];
		return prg[mapVal + (offsetAddress % K_8)];
	}
	else if (RAM_ENABLED)// Read PRG RAM
	{
		u16 offsetAddress = (address - K_24);
		return prgRAM[offsetAddress];
	}

} // read8()

u8 Mapper1::write8(u16 address, u8 val)
{
	// PRG RAM Write
	if (address < K_32 && RAM_ENABLED)
	{
		prgRAM[address - K_24] = val;
	}
	// Mapper Register Write
	else if (address & K_32)
	{
		// Reset Shift Register and Write Control
		if (val & 0x80)
		{
			control			|= 0x0C;
			writeCount		= 0;
			shiftRegister	= 0;
			SetBanks();
		}
		else
		{
			// Write a bit into the temporary Register
			shiftRegister >>= 1;
			shiftRegister |= (val & 0x01) << 4;
			writeCount++;

			// Finished Writing all the Bits
			if ( writeCount >= 5 )
			{
				int regIndex = (address >> 13) & 0x03; //  internal register selected by bits 14 and 13 of the address
				switch (regIndex)
				{
				case 0:
					control = shiftRegister; 
					break;
				case 1:
					chrBank0 = shiftRegister;
					break;
				case 2:
					chrBank1 = shiftRegister;
					break;
				case 3:
					prgBank = shiftRegister;
					break;
				} // switch
				writeCount = 0;
				shiftRegister = 0;
				SetBanks();
			}
		}

	}
	return val;

} // write8()

u8 Mapper1::chr_write8(u16 address, u8 val)
{
	return chr[address] = val;

} // chr_write8()

MAPPER::SaveData Mapper1::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();

	savedMapper.mapperData[0]	= control;
	savedMapper.mapperData[1]	= prgBank;
	savedMapper.mapperData[2]	= chrBank0;
	savedMapper.mapperData[3]	= chrBank1;
	savedMapper.mapperData[4]	= shiftRegister;
	savedMapper.mapperData[5]	= writeCount;

	return savedMapper;

} // GrabSaveData()

void Mapper1::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);

	control			= loadedData.mapperData[0];
	prgBank			= loadedData.mapperData[1];
	chrBank0		= loadedData.mapperData[2];
	chrBank1		= loadedData.mapperData[3];
	shiftRegister	= loadedData.mapperData[4];
	writeCount		= loadedData.mapperData[5];

	SetBanks();

} // LoadSaveData()
