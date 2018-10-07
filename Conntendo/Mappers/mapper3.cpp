#include "ppu.h"
#include "mapper3.h"

// Constructor
Mapper3::Mapper3(u8* rom) : Mapper(rom),
	shiftRegister(0x0),
	vertMirroring(rom[6] & 0x01)
{
	MapPRG(16, 0, 0); // CPU $8000 - $FFFF: 16 KB PRG ROM, fixed (if 16 KB PRG ROM used, then this is the same as $C000 - $FFFF)
	MapPRG(16, 1, 1); // CPU $C000-$FFFF: 16 KB PRG ROM, fixed

	PPU::Mirroring mode = (vertMirroring) ? PPU::Mirroring::VERTICAL : PPU::Mirroring::HORIZONTAL;
	SetMirrorMode(mode); // Fixed to SolderPad on ROM

	SetBanks();
}

void Mapper3::SetBanks()
{
	MapCHR( 8, 0, shiftRegister); // PPU $0000-$1FFF: 8 KB switchable CHR ROM bank

} // SetBanks()

u8 Mapper3::write8(u16 address, u8 val)
{ 
	if ( address >= 0x8000 ) // Bank Switch
	{
		shiftRegister = val; // CNROM only implements the lowest 2 bits (rest do more)
		SetBanks();
	}
	return val;

} // write8()

u8 Mapper3::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()

MAPPER::SaveData Mapper3::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();
	savedMapper.mapperData[0] = shiftRegister;
	savedMapper.mapperData[1] = vertMirroring;
	return savedMapper;

} // GrabSaveData()

void Mapper3::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);
	shiftRegister = loadedData.mapperData[0];
	vertMirroring = loadedData.mapperData[1];
	SetBanks();

} // LoadSaveData()
