#pragma once
#include "mapper.h"

// Color Dreams/ Wisdom Tree ROMs
class Mapper11 : public Mapper
{
public:
	Mapper11(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	u8 bankSelect; 
};
