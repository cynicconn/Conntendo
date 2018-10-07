#pragma once
#include "mapper.h"

// Sunsoft FME-7 ROMs ( Batman Return of the Joker )
class Mapper69 : public Mapper
{
public:
	Mapper69(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 read8(u16 address);
	u8 write8(u16 address, u8 val);
	u8 chr_write8(u16 address, u8 val);

	// CPU IRQ
	void SignalCPU();

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	u8 command;
	u8 parameter;

	u8 prgBank[4];
	u8 chrBank[8];

	u8 ntMirror;

	// IRQ Variables
	bool irqEnable;
	bool irqCounterEnable;
	int irqCounter;

	void Command();

};
