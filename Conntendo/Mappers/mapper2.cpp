#include "mapper2.h"
#include "ppu.h"

// Constructor
Mapper2::Mapper2(u8* rom) : Mapper(rom),
	shiftRegister(0x0),
	vertMirroring(rom[6] & 0x01)
{
	PPU::Mirroring mode = (vertMirroring) ? PPU::Mirroring::VERTICAL : PPU::Mirroring::HORIZONTAL;
	SetMirrorMode(mode); // Fixed to SolderPad on ROM

	SetBanks();
}

void Mapper2::SetBanks()
{
	MapPRG(16,  0, shiftRegister & 0x0F );	// CPU $8000-$BFFF: 16 KB Switchable PRG ROM Bank
	MapPRG(16,  1, -1 );					// CPU $C000-$FFFF: 16 KB Fixed PRG ROM Bank, fixed to the last bank
	MapCHR( 8, 0, 0);						// 8K CHR ROM

} // SetBanks()

u8 Mapper2::write8(u16 address, u8 val)
{ 
	if (address >= 0x8000) // Bank Switch
	{
		shiftRegister = (val & 0x07);
		SetBanks();
	}
	return val;

} // write8()

u8 Mapper2::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

MAPPER::SaveData Mapper2::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();
	savedMapper.mapperData[0] = shiftRegister;
	savedMapper.mapperData[1] = vertMirroring;
	return savedMapper;

} // GrabSaveData()

void Mapper2::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);
	shiftRegister = loadedData.mapperData[0];
	vertMirroring = loadedData.mapperData[1];
	SetBanks();

} // LoadSaveData()
