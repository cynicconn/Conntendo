#include "ppu.h"
#include "mapper7.h"

// Constructor
Mapper7::Mapper7(u8* rom) : Mapper(rom)
{
	shiftRegister = 0;
	SetBanks();
}

void Mapper7::SetBanks()
{
	MapPRG(32, 0, shiftRegister & 0x07);	// CPU $8000-$FFFF: 32 KB switchable PRG ROM bank
	MapCHR( 8, 0, 0);						// PPU $0000-$1FFF: 8 KB fixed CHR ROM bank

	PPU::Mirroring mode = (shiftRegister & 0x10) ? PPU::Mirroring::ONESCREEN_UP : PPU::Mirroring::ONESCREEN_LOW;
	SetMirrorMode(mode);

} // SetBanks()

u8 Mapper7::write8(u16 address, u8 val)
{ 
	if ( address >= 0x8000 ) // Bank Switch
	{
		shiftRegister = val; // Select 32 KB PRG ROM bank for CPU $8000-$FFFF
		SetBanks();
	}
	return val;

} // write8()

u8 Mapper7::chr_write8(u16 address, u8 val)
{ 
	return chr[address] = val;

} // chr_write8()
