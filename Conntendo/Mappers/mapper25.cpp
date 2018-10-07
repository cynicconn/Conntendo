#include "mapper25.h"

// Conntendo
#include "cpu.h"
#include "ppu.h"

// STILL WORK IN PROGRESS ( CHR Mapping is Broken)
#define SIGNAL_COUNT	341
#define MIRROR_MODE		(val & 0x03)
#define PRG_SELECT		(val & 0x1F)
#define PRG_SWAP		(val & 0x02)
#define CHR_SLOT		((address >> 1) & 0x01) | ((address - 0xB000) >> 11)

// Constructor
Mapper25::Mapper25(u8* rom) : Mapper(rom)
{
	prgSelect0			= 0;
	prgSelect1			= 0;
	mirroring			= 0;
	prgSwapMode			= 0;
	memset(chrSelect, 0, 8 * sizeof(u16));

	// IRQ Stuff
	irqLatch		= 0;
	irqControl		= 0;
	irqPreScaler	= SIGNAL_COUNT;
	irqCounter		= 0;
	irqAck			= 0;
	irqMode			= 0;

	SetBanks();
}

void Mapper25::SetBanks()
{
	// Assign all eight of the 1 KB CHR Banks
	for (int i = 0; i < 8; i++)
	{
		MapCHR(1, i, chrSelect[i]); 

	} // for

	// PRG Banks
	if (!prgSwapMode) // Clear
	{
		MapPRG(8, 0, prgSelect0);	// CPU $8000-$9FFF is controlled by the $800x register
		MapPRG(8, 2, -2);			// CPU $C000-$DFFF is fixed to the second last 8 KiB in the ROM
	}
	else // Set
	{
		MapPRG(8, 0, -2);			// CPU $8000-$9FFF is fixed to the second last 8 KiB in the ROM
		MapPRG(8, 2, prgSelect0);	// CPU $C000-$DFFF is controlled by the $800x register
	}

	MapPRG(8, 1, prgSelect1);		// CPU $A000-$BFFF: 8 KiB switchable PRG ROM bank
	MapPRG(8, 3, -1);				// CPU $E000-$FFFF: 8 KiB PRG ROM bank, fixed to the last bank

	// Mirror Mode
	switch (mirroring)
	{
	case 0:
		SetMirrorMode(PPU::VERTICAL);
		break;
	case 1:
		SetMirrorMode(PPU::HORIZONTAL);
		break;
	case 2:
		SetMirrorMode(PPU::ONESCREEN_LOW);
		break;
	case 3:
		SetMirrorMode(PPU::ONESCREEN_UP);
		break;
	}

} // SetBanks()

// According to Documentation, it really IS 9-Bits (4 low, 5 high)
void Mapper25::UpdateCHRSelect(u8 val, bool writeToHigh, u8 slot)
{
	if (writeToHigh) // Low
	{
		u16 low = chrSelect[slot];
		u16 high = (val & 0x1F) << 4;
		chrSelect[slot] = low | high;
	}
	else // High
	{
		chrSelect[slot] = (chrSelect[slot] & 0x01F0) | (val & 0x0F);
	}

} // UpdateCHRSelect()

u8 Mapper25::write8(u16 address, u8 val)
{ 
	// CHR Select
	if (address >= 0xB000 && address <= 0xE003)
	{
		bool writeToHigh = (address & 0x01);
		u8 chrSlot = CHR_SLOT;
		UpdateCHRSelect(val, writeToHigh, chrSlot);
	}
	else if (address < K_32)
	{
		prgRAM[address - K_24] = val;
	}
	else if (address == 0x9000 || address == 0x9001) // Mirroring Control
	{
		mirroring = MIRROR_MODE;
	}
	else if (address == 0x9002 || address == 0x9003) // PRG Swap Mode
	{
		prgSwapMode = PRG_SWAP;
	}
	else if (address >= 0x8000 && address <= 0x8003) // PRG Select 0
	{
		prgSelect0 = PRG_SELECT;
	}
	else if (address >= 0xA000 && address <= 0xA003) // PRG Select 1
	{
		prgSelect1 = PRG_SELECT;
	}

	// IRQ
	if (address == 0xF000) // IRQ Latch, low 4 bits
	{
		CPU::Clear_IRQ();
		irqLatch &= 0xF0; 
		irqLatch |= val & 0x0F;
	}
	else if (address == 0xF001) // IRQ Latch, high 4 bits
	{
		CPU::Clear_IRQ();
		irqLatch &= 0x0F;
		irqLatch |= val << 4;
	}
	else if (address == 0xF002) // IRQ Control
	{
		CPU::Clear_IRQ();
		irqPreScaler	= SIGNAL_COUNT;
		irqCounter		= irqLatch;
		irqControl		= val & 0x1;
		irqAck			= val & 0x2;
		irqMode			= val & 0x4;
	}
	else if (address == 0xF003) // IRQ Acknowledge
	{
		CPU::Clear_IRQ();
		irqAck = irqControl;
	}

	SetBanks();
	return val;

} // write8()

u8 Mapper25::chr_read8(u16 address)
{
	u8 slot = address / K_1;
	u32 mapAddr = chrMap[slot] + (address % K_1);
	return chr[mapAddr];

} // chr_read8()

u8 Mapper25::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

void Mapper25::ClockedIRQ()
{
	irqCounter++;
	// HACK For some reason, 222 was value that lined up HUD in Bio Miracle Baby
	if (irqCounter == 222 )
	{
		irqCounter = irqLatch;
		CPU::Set_IRQ();
	}
} // ClockedIRQ()

void Mapper25::SignalCPU()
{
	if (irqAck)
	{
		if (irqMode) // Cycle Mode
		{
			ClockedIRQ();
		}
		else // Scanline Mode
		{
			irqPreScaler -= 3;
			if (irqPreScaler <= 0)
			{
				irqPreScaler += SIGNAL_COUNT;
				ClockedIRQ();
			}
		}
	}

} // SignalCPU()

MAPPER::SaveData Mapper25::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();

	savedMapper.mapperData[0] = prgSelect0;
	savedMapper.mapperData[1] = prgSelect1;
	savedMapper.mapperData[2] = prgSwapMode;
	savedMapper.mapperData[3] = mirroring;
	savedMapper.mapperData[4] = irqLatch;
	savedMapper.mapperData[5] = irqControl;
	savedMapper.mapperData[6] = irqAck;
	savedMapper.mapperData[7] = irqMode;

	savedMapper.mapperData[8] = irqPreScaler; 
	savedMapper.mapperData[9] = irqPreScaler >> 8; 

	savedMapper.mapperData[10] = irqCounter;
	savedMapper.mapperData[11] = irqCounter >> 8;

	int startNum = 12;
	int curBank = 0;
	for (int i = 0; i < 16; i += 2 )
	{
		savedMapper.mapperData[startNum+i] = chrSelect[curBank];
		savedMapper.mapperData[startNum+i+1] = chrSelect[curBank] >> 8;
		curBank++;
	} // for

	return savedMapper;

} // GrabSaveData()

void Mapper25::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);

	prgSelect0 = loadedData.mapperData[0];
	prgSelect1 = loadedData.mapperData[1];
	prgSwapMode = loadedData.mapperData[2];
	mirroring = loadedData.mapperData[3];
	irqLatch = loadedData.mapperData[4];
	irqControl = loadedData.mapperData[5];
	irqAck = loadedData.mapperData[6];
	irqMode = loadedData.mapperData[7];

	irqPreScaler	= loadedData.mapperData[8] | (loadedData.mapperData[9] << 8);
	irqCounter		= loadedData.mapperData[10] | (loadedData.mapperData[11] << 8);

	int startNum = 12;
	int curBank = 0;
	for (int i = 0; i < 16; i += 2)
	{
		chrSelect[curBank] = loadedData.mapperData[startNum+i] | (loadedData.mapperData[startNum+i+1] << 8);
		curBank++;
	} // for

	SetBanks();

} // LoadSaveData()
