#pragma once
#include "mapper.h"

// CURRENTLY WORK IN PROGRESS (CHR Mapping is faulty)
// VRC4 Konami ROMs
class Mapper25 : public Mapper
{
public:
	Mapper25(u8* rom);
	void SetBanks();

	// Read-Write Functions
	u8 write8(u16 address, u8 val);
	u8 chr_read8(u16 address);
	u8 chr_write8(u16 address, u8 val);

	// IRQ Signal
	void SignalCPU();

	// SaveStates
	MAPPER::SaveData GrabSaveData();
	void LoadSaveData(MAPPER::SaveData loadedData);

private:
	// Mapper Registers
	u8 prgSelect0; 
	u8 prgSelect1;
	bool prgSwapMode;
	u8 mirroring;
	u16 chrSelect[8];

	// IRQ Variables
	u8 irqLatch;
	bool irqControl;
	bool irqAck;
	bool irqMode;
	s16 irqPreScaler;
	s16 irqCounter;

	// Helper Functions
	void ClockedIRQ();
	void UpdateCHRSelect(u8 val, bool writeToHigh, u8 slot);
};
