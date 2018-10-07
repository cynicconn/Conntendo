#include "mapper66.h"
#include "ppu.h"

#define PRG_SHIFT (shiftRegister >> 4)
#define CHR_SHIFT (shiftRegister & 0x03)

// Constructor
Mapper66::Mapper66(u8* rom) : Mapper(rom)
{
	shiftRegister = 0;
	SetBanks();
}

void Mapper66::SetBanks()
{
	MapPRG(32,  0, PRG_SHIFT);		// CPU $8000-$FFFF: 32 KB switchable PRG ROM bank
	MapCHR( 8,   0, CHR_SHIFT);		// PPU $0000-$1FFF: 8 KB switchable CHR ROM bank

} // SetBanks()

u8 Mapper66::write8(u16 address, u8 val)
{ 
	// Bank Switch
	if (address >= 0x8000) 
	{
		shiftRegister = (val & 0x33 );
		SetBanks();
	}
	return val;

} // write8()

u8 Mapper66::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

MAPPER::SaveData Mapper66::GrabSaveData()
{
	MAPPER::SaveData savedMapper	= Mapper::GrabSaveData();
	savedMapper.mapperData[0]		= shiftRegister;
	return savedMapper;

} // GrabSaveData()

void Mapper66::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);
	shiftRegister = loadedData.mapperData[0];
	SetBanks();

} // LoadSaveData()
