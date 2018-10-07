#pragma once
#include "mapper.h"

// NES AxROMs for RARE games: RC-ProAM, Battletoads etc
class Mapper7 : public Mapper
{
public:
	Mapper7(u8* rom);
	void SetBanks();

	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

private:
	u8 shiftRegister;
};
