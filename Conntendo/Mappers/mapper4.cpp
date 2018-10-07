#include "ppu.h"
#include "cpu.h"
#include "mapper4.h"

#define PRG_BANK_MODE	(bankSelect & 0x40)
#define CHR_INVERSION	(bankSelect & 0x80)
#define MAP_INDEX		(bankSelect & 0x07)
#define MIRROR_MODE		(val & 0x01)

// Constructor
Mapper4::Mapper4(u8* rom) : Mapper(rom)
{
	memset(bankData, 0, sizeof(bankData[0]));
	bankSelect = 0;
	horMirroring = true;

	irqLatch = 0;
	irqReload = 0;
	irqDisable = false;
	irqEnable = false;

	MapPRG( 8, 3, -1); // CPU $E000 - $FFFF: 8 KB PRG ROM bank, always fixed to the last bank
	SetBanks();
}

void Mapper4::SetBanks()
{
	// PRG Mode 0 or 1
	if (!PRG_BANK_MODE)
	{
		MapPRG( 8, 0, bankData[6]);
		MapPRG( 8, 2, -2);
	}
	else
	{
		MapPRG( 8, 0, -2);
		MapPRG( 8, 2, bankData[6]);
	}

	// Swap CHR and PRG Map
	u8 slot;
	switch (MAP_INDEX)
	{
	case 0:
		slot = CHR_INVERSION ? 2 : 0;
		MapCHR( 2, slot, bankData[0] >> 1); // PPU $0000-$07FF (or $1000-$17FF): 2 KB switchable CHR bank
		break;
	case 1:
		slot = CHR_INVERSION ? 3 : 1;
		MapCHR( 2, slot, bankData[1] >> 1); // PPU $0800-$0FFF (or $1800-$1FFF): 2 KB switchable CHR bank
		break;
	case 2:
		slot = CHR_INVERSION ? 0 : 4;
		MapCHR( 1, slot, bankData[2]); // PPU $1000-$13FF (or $0000-$03FF): 1 KB switchable CHR bank
		break;
	case 3:
		slot = CHR_INVERSION ? 1 : 5;
		MapCHR( 1, slot, bankData[3]); // PPU $1400-$17FF (or $0400-$07FF): 1 KB switchable CHR bank
		break;
	case 4:
		slot = CHR_INVERSION ? 2 : 6;
		MapCHR( 1, slot, bankData[4]); // PPU $1800-$1BFF (or $0800-$0BFF): 1 KB switchable CHR bank
		break;
	case 5:
		slot = CHR_INVERSION ? 3 : 7;
		MapCHR( 1, slot, bankData[5]); // PPU $1C00-$1FFF (or $0C00-$0FFF): 1 KB switchable CHR bank
		break;
	case 7:
		MapPRG( 8, 1, bankData[7]); // CPU $A000 - $BFFF: 8 KB switchable PRG ROM bank
		break;
	} // switch

	PPU::Mirroring mode = (horMirroring) ? PPU::Mirroring::HORIZONTAL : PPU::Mirroring::VERTICAL;
	SetMirrorMode(mode); 

} // SetBanks()

u8 Mapper4::write8(u16 address, u8 val)
{ 
	if (address < 0x8000)
	{
		prgRAM[address - K_24] = val;
	}
	else if (address & 0x8000)
	{
		switch (address & 0xE001)
		{
		case 0x8000:  
			bankSelect = val;
			break;
		case 0x8001:  
			bankData[MAP_INDEX] = val;
			break;
		case 0xA000:
			horMirroring = MIRROR_MODE;
			break;
		case 0xC000:
			irqLatch = val;	
			break;
		case 0xC001:
			irqReload = 0;
			break;
		case 0xE000:
			CPU::Clear_IRQ(); 
			irqEnable = false;
			break;
		case 0xE001:
			irqEnable = true;	
			break;
		}
		SetBanks();
	}
	return val;

} // write8()

u8 Mapper4::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

// Handle Scanline-Based IRQ Signal
void Mapper4::SignalScanline()
{
	if (irqReload == 0)
	{
		irqReload = irqLatch;
	}
	else
	{
		irqReload--;
	}

	if (irqEnable && irqReload == 0 )
	{
		CPU::Set_IRQ();
	}

} // SignalScanline()

// Read 4K NameTable RAM
u8 Mapper4::ReadExtraRAM(u16 address)
{
	return extraRAM[address & 0x0FFF];

} // ReadExtraRAM()

// Write 4K NameTable RAM
u8 Mapper4::WriteExtraRAM(u16 address, u8 val)
{
	return extraRAM[address & 0x0FFF] = val;

} // WriteExtraRAM()

MAPPER::SaveData Mapper4::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();

	savedMapper.mapperData[0]	= bankSelect;
	savedMapper.mapperData[1]	= bankData[0];
	savedMapper.mapperData[2]	= bankData[1];
	savedMapper.mapperData[3]	= bankData[2];
	savedMapper.mapperData[4]	= bankData[3];
	savedMapper.mapperData[5]	= bankData[4];
	savedMapper.mapperData[6]	= bankData[5];
	savedMapper.mapperData[7]	= bankData[6];
	savedMapper.mapperData[8]	= bankData[7];
	savedMapper.mapperData[9]	= horMirroring;
	savedMapper.mapperData[10]	= irqLatch;
	savedMapper.mapperData[11]	= irqReload;
	savedMapper.mapperData[12]	= irqDisable;
	savedMapper.mapperData[13]	= irqEnable;

	for (int i = 0; i < K_4; i++)
	{
		savedMapper.extraRAM[i] = extraRAM[i];
	} // for

	return savedMapper;

} // GrabSaveData()

void Mapper4::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);

	bankSelect		= loadedData.mapperData[0];
	bankData[0]		= loadedData.mapperData[1];
	bankData[1]		= loadedData.mapperData[2];
	bankData[2]		= loadedData.mapperData[3];
	bankData[3]		= loadedData.mapperData[4];
	bankData[4]		= loadedData.mapperData[5];
	bankData[5]		= loadedData.mapperData[6];
	bankData[6]		= loadedData.mapperData[7];
	bankData[7]		= loadedData.mapperData[8];
	horMirroring	= loadedData.mapperData[9];
	irqLatch		= loadedData.mapperData[10];
	irqReload		= loadedData.mapperData[11];
	irqDisable		= loadedData.mapperData[12];
	irqEnable		= loadedData.mapperData[13];

	for (int i = 0; i < K_4; i++)
	{
		extraRAM[i] = loadedData.extraRAM[i];
	} // for

	SetBanks();

} // LoadSaveData()
