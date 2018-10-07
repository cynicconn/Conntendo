#pragma once
#include "mapper.h"

// NES CNROMs for early games like Gradius and Arkanoid
// CHR Bank Swapping Only
class Mapper3 : public Mapper
{
public:
	Mapper3(u8* rom);
	void SetBanks();

	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	u8 shiftRegister; 
	bool vertMirroring;
};
