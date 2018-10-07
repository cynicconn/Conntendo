#include "cartridge.h"

// Conntendo
#include "cpu.h"
#include "apu.h"
#include "ppu.h"
#include "joypad.h"
#include "emulator.h"
#include "files.h"

// Mappers
#include "mapper0.h"
#include "mapper1.h"
#include "mapper2.h"
#include "mapper3.h"
#include "mapper4.h"
#include "mapper5.h"
#include "mapper7.h"
#include "mapper9.h"
#include "mapper10.h"
#include "mapper11.h"
#include "mapper25.h"
#include "mapper66.h"
#include "mapper69.h"

// iNES Extract 
#define EXTRACT_MAPPER		(rom[7] & 0xF0) | (rom[6] >> 4)
#define IGNORE_MIRRORING	(rom[6] & 0x08)

namespace Cartridge
{
	Mapper* mapper		= nullptr; 
	string gameName		= "";

	// Get name of current game loaded
	string GetGameName()
	{
		return gameName;

	} // GetGameName()

	// Return path to Cartridge's Mapper Chip
	Mapper* GetMapper() 
	{
		return mapper;

	} // GetMapper() 

	u8 WritePRG(u16 address, u8 val)
	{
		return mapper->write8(address, val);

	} // WritePRG()

	u8 ReadPRG(u16 address)
	{
		return mapper->read8(address);

	} // ReadPRG()

	u8 WriteCHR(u16 address, u8 val)
	{
		return mapper->chr_write8(address, val);

	} // WriteCHR()

	u8 ReadCHR(u16 address)
	{
		return mapper->chr_read8(address);

	} // ReadCHR()

	// For CPU-Counter IRQs 
	void SignalCPU()
	{
		mapper->SignalCPU();

	} // SignalCPU()	
	
	// For Scanline-Counter IRQs
	void SignalScanline()
	{
		mapper->SignalScanline();

	} // SignalScanline()

	u8 ReadExtraRAM(u16 address)
	{
		return mapper->ReadExtraRAM(address);

	} // ReadExtraRAM()

	u8 WriteExtraRAM(u16 address, u8 val)
	{
		return mapper->WriteExtraRAM(address, val);

	} // WriteExtraRAM()

	bool CreateSaveState(int slot)
	{
		string filePath = Emulator::GetSavePath() + gameName + SAVE_EXT;
		return Files::CreateSaveState(filePath);

	} // CreateSaveState()

	bool LoadSaveState(int slot)
	{
		string filePath = Emulator::GetSavePath() + gameName + SAVE_EXT;
		return Files::LoadSaveState(filePath);

	} // LoadSaveState()

	// Get Name of Game from full file path
	void ExtractGameName(const char* romPath)
	{
		std::string truncatedName(romPath);
		truncatedName = truncatedName.substr(0, truncatedName.length() - 4); // remove ".nes" extension
		while (truncatedName.find("/") != string::npos)
		{
			int getTo = truncatedName.find("/");
			truncatedName = truncatedName.substr(getTo + 1, truncatedName.length());
		} // while
		while (truncatedName.find("\\") != string::npos)
		{
			int getTo = truncatedName.find("\\");
			truncatedName = truncatedName.substr(getTo + 1, truncatedName.length());
		} // while
		gameName = truncatedName;

	} // ExtractGameName()

	// Load NES game ROM from File
	bool LoadROM( const char* romPath )
	{
		// Attempt to read ROM File
		u8* rom = Files::ReadROMFile(romPath);
		if ( rom == nullptr )
		{
			Emulator::ShowMessage("Error Grabbing ROM");
			return false;
		}

		ExtractGameName(romPath);

		// Get Mapper Num for ROM File ( assuming iNES format )
		int mapperNum = EXTRACT_MAPPER;

		// Special Case games that use 4K Nametable RAM
		bool useExtraRAM = IGNORE_MIRRORING;
		PPU::DisableCIRAM(useExtraRAM);

		// Cleanup previous Mapper data before loading for new Cartridge
		if (Emulator::IsLoaded())
		{
			delete mapper;
		}

		// Instantiate to appropriate Mapper if it exists
		switch (mapperNum)
		{
		case 0: // Stock
			mapper = new Mapper0(rom); 
			break;
		case 1: // MMC1
			mapper = new Mapper1(rom);
			break;
		case 2: // UxROM
			mapper = new Mapper2(rom);
			break;
		case 3: // CNROM
			mapper = new Mapper3(rom);
			break;
		case 4: // MMC3
			mapper = new Mapper4(rom);
			break;
		case 5: // MMC5
			mapper = new Mapper5(rom);
			break;
		case 7: // AxROM 
			mapper = new Mapper7(rom);
			break;
		case 9: // MMC2
			mapper = new Mapper9(rom);
			break;
		case 10: // MMC4
			mapper = new Mapper10(rom);
			break;
		case 11: // Color Dreams
			mapper = new Mapper11(rom); 
			break;
		case 25: // VRC4 (Work-in-Progress)
			mapper = new Mapper25(rom);
			break;
		case 66: // GxROM
			mapper = new Mapper66(rom);
			break;
		case 69: // Sunsoft FME-7
			mapper = new Mapper69(rom);
			break;
		default: // Mapper does not exist yet
			return false;
		}
		
		// ROM Successfully Loaded
		return true;

	} // LoadROM()

} // Cartridge