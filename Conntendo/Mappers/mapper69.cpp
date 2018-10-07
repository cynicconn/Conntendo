#include "mapper69.h"

// Conntendo
#include "cpu.h"
#include "ppu.h"

#define RAM_ENABLED (prgBank[0] & 0x80)
#define ROM_BIT		(!(prgBank[0] & 0x40))

// Constructor
Mapper69::Mapper69(u8* rom) : Mapper(rom)
{
	command = 0;
	parameter = 0;
	ntMirror = 0;

	irqEnable = false;
	irqCounterEnable = false;
	irqCounter = 0;

	memset(prgBank, 0, 4 * sizeof(u8));
	memset(chrBank, 0, 8 * sizeof(u8));

	SetBanks();
}

void Mapper69::SetBanks()
{
	// PRG Banks
	MapPRG(8, 0, prgBank[1]);
	MapPRG(8, 1, prgBank[2]);
	MapPRG(8, 2, prgBank[3]);
	MapPRG(8, 3, -1);

	// CHR Banks
	for (int i = 0; i < 8; i++)
	{
		MapCHR(1, i, chrBank[i] );
	} // for

	// Mirror Mode
	switch (ntMirror)
	{
	case 0:
		SetMirrorMode(PPU::VERTICAL);
		break;
	case 1:
		SetMirrorMode(PPU::HORIZONTAL);
		break;
	case 2:
		SetMirrorMode(PPU::ONESCREEN_LOW); // $2000 ("1ScA")
		break;
	case 3:
		SetMirrorMode(PPU::ONESCREEN_UP); // $2400 ("1ScB")
		break;
	}


} // SetBanks()

// Execute Mapper Command with Parameter
void Mapper69::Command()
{
	if (command >= 0 && command <= 7) // CHR Banks
	{
		chrBank[command] = parameter;
	}
	else if (command == 8) // Prg Bank0
	{
		prgBank[0] = parameter;
		// TODO: RAM
	}
	else if (command >= 9 && command <= 0xB) // Prg Bank0
	{
		int slot = command - 8;
		prgBank[slot] = parameter & 0x3F;
	}
	else if (command == 0xC ) // Nametable Mirroring
	{
		ntMirror = parameter & 0x3;
	}
	else if (command == 0xD) // IRQ Control
	{
		CPU::Clear_IRQ();
		irqEnable = parameter & 0x01;
		irqCounterEnable = parameter & 0x80;
	}
	else if (command == 0xE) // IRQ Counter Low Byte 
	{
		irqCounter = parameter;
	}
	else if (command == 0xF) // IRQ Counter High Byte
	{
		irqCounter = (irqCounter & 0xFF) | ( parameter << 8 );
	}

} // Command()

u8 Mapper69::read8(u16 address)
{
	if (address < K_32) 
	{
		if (RAM_ENABLED) // PRG RAM
		{
			u16 offsetAddress = (address - K_24);
			return prgRAM[offsetAddress];
		}
		else // PRG ROM instead of RAM
		{
			u8 bankAddr = prgBank[0] & 0x3F;
			u32 addr = (8 * K_1 * bankAddr) % prgSize;
			return prg[addr];
		}
	}
	else if (address >= K_32) // PRG ROM
	{
		u16 offsetAddress = (address - K_32);
		u32 prgAddr = prgMap[offsetAddress / K_8] + (offsetAddress % K_8);
		return prg[prgAddr];
	}

} // read8()

u8 Mapper69::write8(u16 address, u8 val)
{ 
	// PRG RAM Write
	if (address < K_32 && RAM_ENABLED)
	{
		prgRAM[address - K_24] = val;
	}

	else if (address >= 0x8000 && address <= 0x9FFF) 
	{
		command = val & 0x0F;
	}
	else if (address >= 0xA000 && address <= 0xBFFF) 
	{
		parameter = val;
		Command();
	}
	SetBanks();

	return val;

} // write8()

u8 Mapper69::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

// CPU-Based IRQ System
void Mapper69::SignalCPU()
{
	if (irqCounterEnable)
	{
		irqCounter--;
		if (irqEnable && irqCounter <= 0)
		{
			CPU::Set_IRQ();
			irqCounter = 0xFFFF;
		}
	}

} // SignalCPU()

MAPPER::SaveData Mapper69::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();

	for (int i = 0; i < 8; i++)
	{
		savedMapper.mapperData[i] = chrBank[i];
	} // for

	for (int i = 8; i < 12; i++)
	{
		savedMapper.mapperData[i] = prgBank[i];
	} // for

	savedMapper.mapperData[12] = command;
	savedMapper.mapperData[13] = parameter;
	savedMapper.mapperData[14] = ntMirror;
	savedMapper.mapperData[15] = irqEnable;
	savedMapper.mapperData[16] = irqCounterEnable;

	savedMapper.mapperData[17] = irqCounter;
	savedMapper.mapperData[18] = irqCounter >> 8;
	savedMapper.mapperData[19] = irqCounter >> 16;
	savedMapper.mapperData[20] = irqCounter >> 24;

	return savedMapper;

} // GrabSaveData()

void Mapper69::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);

	for (int i = 0; i < 8; i++)
	{
		chrBank[i] = loadedData.mapperData[i];
	} // for

	for (int i = 8; i < 12; i++)
	{
		prgBank[i] = loadedData.mapperData[i];
	} // for

	command = loadedData.mapperData[12];
	parameter = loadedData.mapperData[13];
	ntMirror = loadedData.mapperData[14];
	irqEnable = loadedData.mapperData[15];
	irqCounterEnable = loadedData.mapperData[16];

	irqCounter = (loadedData.mapperData[17]) | (loadedData.mapperData[18] << 8) 
		| (loadedData.mapperData[19] << 16) | (loadedData.mapperData[20] << 24);

	SetBanks();

} // LoadSaveData()
