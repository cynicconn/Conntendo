#pragma once
//----------------------------------------------------------------//
// Audio System for NES: reading and writing sound data from Cartridge to APU
// Actual sound synthesis provided by Shay Green's Sound Generation Library
//----------------------------------------------------------------//

// Conntendo
#include "common.h"

// Blargg Audio Library
#include "nes_apu/Nes_Apu.h"
#include "Sound_Queue.h"

namespace APU
{
	// Blargg Audio
	void OutputSamples(const blip_sample_t* samples, size_t count);
	int DMCRead(void*, cpu_addr_t address);

	// Read|Write Functions
	u8 write8( long elapsed, u16 address, u8 val );
	u8 read8( long elapsed );

	// Managing APU
	void Init();
	void Reset();
	void RunFrame( long length );

	// Emulator Seetings
	bool ToggleMuteAudio();
	bool ToggleOneChannel(int channel);
	void AdjustVolume(float adjust);
	string PrintVolume();

} // APU

