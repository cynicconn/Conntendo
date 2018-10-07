#pragma once
#include "mapper.h"

// Stock NES Hardware
class Mapper0 : public Mapper
{
public:
	Mapper0(u8* rom) : Mapper(rom)
	{
		MapPRG( 32, 0, 0); // 16K or 32K PRG_ROM
		MapCHR( 8,  0, 0); // 8K  CHR ROM
	}
};
