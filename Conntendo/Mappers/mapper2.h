#pragma once
#include "mapper.h"

// NES-UxROMs such as Megaman, Castlevania, and Contra
class Mapper2 : public Mapper
{
public:
	Mapper2(u8* rom);
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
