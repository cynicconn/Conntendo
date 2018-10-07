#pragma once
#include "mapper.h"

// NES-GxROMs for Dragon Power, Gumshoe etc
class Mapper66 : public Mapper
{
public:
	Mapper66(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	u8 shiftRegister; 
};
