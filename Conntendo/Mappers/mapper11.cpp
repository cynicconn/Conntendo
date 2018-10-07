#include "ppu.h"
#include "mapper11.h"

// Constructor
Mapper11::Mapper11(u8* rom) : Mapper(rom)
{
	bankSelect = 0;
	SetBanks();
}

void Mapper11::SetBanks()
{
	MapPRG(32,  0, bankSelect & 0x03 );		// CPU $8000-$FFFF: 32 KB Switchable PRG ROM Bank
	MapCHR( 8,  0, bankSelect >> 4 ); 		// PPU $0000-$1FFF: 8 KB Switchable CHR ROM Bank

} // SetBanks()

u8 Mapper11::write8(u16 address, u8 val)
{ 
	// Bank Switch
	if (address >= K_32 && address <= K_64 ) 
	{
		bankSelect = val;
		SetBanks();
	}
	return val;

} // write8()

u8 Mapper11::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

MAPPER::SaveData Mapper11::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();
	savedMapper.mapperData[0] = bankSelect;
	return savedMapper;

} // GrabSaveData()

void Mapper11::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);
	bankSelect = loadedData.mapperData[0];
	SetBanks();

} // LoadSaveData()
