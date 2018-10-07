#include "apu.h"

// Conntendo
#include "cpu.h"

#define DEFAULT_VOLUME 0.5f

namespace APU
{
	// Consts
	const long SAMPLE_RATE	= 96000;   // 96 KHz
	const long CPU_CLOCK	= 1789773; // NTSC 1.789773 MHz
	const size_t OUT_SIZE	= 4096;

	// Blargg Audio
	Blip_Buffer buffer;
	Nes_Apu blarggAPU;
	blip_sample_t outBuf[OUT_SIZE];

	double totalCycles;
	static Sound_Queue* soundQueue;

	// Conntendo Settings
	float gameVolume			= 1;
	bool  bMuteAudio			= false;
	bool  muteChannelList[5]	= { 0 };

	void OutputSamples(const blip_sample_t* samples, size_t count)
	{
		soundQueue->write( samples, count);

	} // OutputSamples()

	// Enable or disable Emulator sound
	bool ToggleMuteAudio()
	{
		bMuteAudio = !bMuteAudio;
		float newVol = (bMuteAudio) ? 0 : gameVolume;
		blarggAPU.volume(newVol);
		return bMuteAudio;

	} // ToggleMuteAudio()

	void AdjustVolume(float adjust)
	{
		gameVolume += adjust;
		gameVolume = CLAMP(gameVolume, 0, 1);
		blarggAPU.volume(gameVolume);

	} // AdjustVolume()

	string PrintVolume()
	{
		string volPrint = "Vol: " + to_string(gameVolume);
		volPrint.erase(volPrint.find('.') + 3, std::string::npos);
		return (volPrint);

	} // PrintVolume()

	void SetVolume( float newVolume)
	{
		if (bMuteAudio)
		{
			return;
		}
		gameVolume = newVolume;
		gameVolume = CLAMP(gameVolume, 0, 1);
		blarggAPU.volume(gameVolume);

	} // SetVolume()

	bool ToggleOneChannel( int channel )
	{
		muteChannelList[channel] = !muteChannelList[channel];
		blarggAPU.mutechannels(muteChannelList);
		return muteChannelList[channel];

	} // ToggleOneChannel()

	// At end of CPU Frame, run APU
	void RunFrame(long length )
	{
		blarggAPU.end_frame(length);
		buffer.end_frame(length);
		totalCycles -= length;

		// Read samples out of Blip_Buffer if there are enough to fill our output buffer
		if (buffer.samples_avail() >= OUT_SIZE)
		{
			size_t count = buffer.read_samples(outBuf, OUT_SIZE);
			OutputSamples(outBuf, count);
		}

	} // RunFrame()

	u8 write8( long elapsed, u16 address, u8 val )
	{
		blarggAPU.write_register( elapsed, address, val );
		return val;

	} // write8()

	u8 read8(long elapsed)
	{
		u8 val = blarggAPU.read_status( elapsed );
		return val;

	} // read8()

	// Callback Function for playing back samples
	int DMCRead( void*, cpu_addr_t address )
	{
		return CPU::ReadMemory( address );

	} // DMCRead()

	void Init()
	{
		soundQueue = new Sound_Queue;
		soundQueue->init(SAMPLE_RATE);

		buffer.sample_rate(SAMPLE_RATE);
		buffer.clock_rate(CPU_CLOCK);

		blarggAPU.output(&buffer);

		// Assign DMC Callback
		blarggAPU.dmc_reader(DMCRead);

		SetVolume(DEFAULT_VOLUME);

	} // Init()

	void Reset()
	{
		blarggAPU.reset();
		buffer.clear();

	} // Reset()

} // APU

