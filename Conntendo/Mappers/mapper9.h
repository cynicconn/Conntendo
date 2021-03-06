#pragma once
#include "mapper.h"

// MMC2 used ONLY for Punch Out
class Mapper9 : public Mapper
{
public:
	Mapper9(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 write8(u16 address, u8 val);
	u8 chr_read8(u16 address);
	u8 chr_write8(u16 address, u8 val);

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:

	u8 prgBankSelect;

	u8 chrBankSelectA;
	u8 chrBankSelectB;

	u8 latchDataA[2];
	u8 latchDataB[2];

	u8 chrLatchA;
	u8 chrLatchB;

	bool horMirroring;
};
