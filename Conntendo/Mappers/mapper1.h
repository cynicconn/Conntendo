#pragma once
#include "mapper.h"

// MMC1 for Zelda, Metroid, Dragon Warrior etc
// also accounts for non-standard MMC1 boards ( SOROM, SUROM etc )
class Mapper1 : public Mapper
{
public:
	Mapper1(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 read8(u16 address);
	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	u8 control;
	u8 prgBank;
	u8 chrBank0;
	u8 chrBank1;

	u8 shiftRegister;
	u8 writeCount;
};