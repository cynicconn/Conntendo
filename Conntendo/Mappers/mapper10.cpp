#include "ppu.h"
#include "mapper10.h"

#define BANK_SELECT (address >> 12)
#define MIRROR_MODE (val & 0x01)
#define LATCH_DATA  (val & 0x1F)
#define PRG_BANK	(val & 0x0F)

// Constructor
Mapper10::Mapper10(u8* rom) : Mapper(rom)
{
	prgBankSelect = 0;
	chrBankSelectA = 0;
	chrBankSelectB = 0;
	chrLatchA = 0xFE;
	chrLatchB = 0xFE;
	horMirroring = (rom[6] & 0x01);
	latchDataA[0] = latchDataA[1] = 0;
	latchDataB[0] = latchDataB[1] = 0;

	SetBanks();
}

void Mapper10::SetBanks()
{
	// PRG Banks (Two 16K Banks)
	MapPRG(16, 0, prgBankSelect); // CPU $8000-$BFFF: 16 KB switchable PRG ROM bank
	MapPRG(16, 1, -1);	// CPU $C000-$FFFF: 16 KB PRG ROM bank, fixed to the last bank

	// CHR Banks (Two 4K Banks)
	MapCHR(4, 0, chrBankSelectA ); // PPU $0000-$0FFF: 4 KB switchable CHR ROM bank
	MapCHR(4, 1, chrBankSelectB ); // PPU $1000-$1FFF: 4 KB switchable CHR ROM bank

	PPU::Mirroring mode = (horMirroring) ? PPU::Mirroring::HORIZONTAL : PPU::Mirroring::VERTICAL;
	SetMirrorMode(mode);

} // SetBanks()

u8 Mapper10::write8(u16 address, u8 val)
{ 
	// Write to RAM
	if (address < 0x8000)
	{
		prgRAM[address - K_24] = val;
	}

	bool slot;
	switch (BANK_SELECT)
	{
	case 0xA: // PRG ROM bank select ($A000-$AFFF)
		prgBankSelect = PRG_BANK;
		break;
	case 0xB: // CHR ROM $FD/0000 Bank select ($B000-$BFFF)
	case 0xC:
		slot = (BANK_SELECT == 0xC);
		latchDataA[slot] = LATCH_DATA;
		if (chrLatchA == 0xFD)
		{
			chrBankSelectA = latchDataA[0];
		}
		else if (chrLatchA == 0xFE)
		{
			chrBankSelectA = latchDataA[1];
		}
		break;
	case 0xD: // CHR ROM $FD/1000 bank select ($D000-$DFFF)
	case 0xE:
		slot = (BANK_SELECT == 0xE);
		latchDataB[slot] = LATCH_DATA;
		if (chrLatchB == 0xFD)
		{
			chrBankSelectB = latchDataB[0];
		}
		else if (chrLatchB == 0xFE)
		{
			chrBankSelectB = latchDataB[1];
		}
		break;
	case 0xF: // Mirroring
		horMirroring = MIRROR_MODE;
		break;
	} // switch

	SetBanks();
	return val;

} // write8()

u8 Mapper10::chr_read8(u16 address)
{ 
	if (address >= 0x0FD8 && address <= 0x0FDF)
	{
		chrLatchA = 0xFD;
		chrBankSelectA = latchDataA[0];
		MapCHR(4, 0, chrBankSelectA);
	}
	else if (address >= 0x0FE0 && address <= 0x0FEF)
	{
		chrLatchA = 0xFE;
		chrBankSelectA = latchDataA[1];
		MapCHR(4, 0, chrBankSelectA);
	}
	else if (address >= 0x1FD0 && address <= 0x1FDF)
	{
		chrLatchB = 0xFD;
		chrBankSelectB = latchDataB[0];
		MapCHR(4, 1, chrBankSelectB);
	}
	else if (address >= 0x1FE0 && address <= 0x1FEF)
	{
		chrLatchB = 0xFE;
		chrBankSelectB = latchDataB[1];
		MapCHR(4, 1, chrBankSelectB);
	}	
	
	u32 chrAddress = chrMap[address / K_1] + (address % K_1);
	return chr[chrAddress];

} // chr_read8()


u8 Mapper10::chr_write8(u16 address, u8 val)
{
	return chr[address] = val;

} // chr_write8()

MAPPER::SaveData Mapper10::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();

	savedMapper.mapperData[0] = prgBankSelect;
	savedMapper.mapperData[1] = chrBankSelectA;
	savedMapper.mapperData[2] = chrBankSelectB;
	savedMapper.mapperData[3] = latchDataA[0];
	savedMapper.mapperData[4] = latchDataA[1];
	savedMapper.mapperData[5] = latchDataB[0];
	savedMapper.mapperData[6] = latchDataB[1];
	savedMapper.mapperData[7] = chrLatchA;
	savedMapper.mapperData[8] = chrLatchB;
	savedMapper.mapperData[9] = horMirroring;

	return savedMapper;

} // GrabSaveData()

void Mapper10::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);

	prgBankSelect	= loadedData.mapperData[0];	
	chrBankSelectA	= loadedData.mapperData[1];	
	chrBankSelectB	= loadedData.mapperData[2];	
	latchDataA[0]	= loadedData.mapperData[3];	
	latchDataA[1]	= loadedData.mapperData[4];	
	latchDataB[0]	= loadedData.mapperData[5];	
	latchDataB[1]	= loadedData.mapperData[6];	
	chrLatchA		= loadedData.mapperData[7];	
	chrLatchB		= loadedData.mapperData[8];	
	horMirroring	= loadedData.mapperData[9];	

	SetBanks();

} // LoadSaveData()
