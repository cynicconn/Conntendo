#include "mapper.h"

// Conntendo
#include "ppu.h"

#define HEADER_SIZE 16

// Constructor
Mapper::Mapper(u8* rom) : rom(rom)
{
	// Read ROM Header to get Cartridge Capacity
	prgSize			= rom[4] * K_16;
	chrSize			= rom[5] * K_8;
	prgRAMSize		= rom[8] ? (rom[8] * K_8) : K_8;
	PPU::SetMirrorMode( ( rom[6] & 0x01 ) ? PPU::VERTICAL : PPU::HORIZONTAL );

	this->prg		= rom + HEADER_SIZE; // Skip Header (16 bytes)
	this->prgRAM	= new u8[prgRAMSize];
	memset(this->prgRAM, 0, prgRAMSize * sizeof(u8)); 

	// Cartridge either has CHR ROM or CHR RAM
	if ( chrSize > 0 ) 
	{
		this->chr = rom + HEADER_SIZE + prgSize;
	}
	else // RAM
	{
		hasChrRAM	= true;
		chrSize		= K_8;
		this->chr	= new u8[chrSize];
		memset(this->chr, 0, chrSize * sizeof(u8)); 
	}

} // Mapper()

// Destructor
Mapper::~Mapper()
{
	delete rom;
	delete prgRAM;
	if (hasChrRAM)
	{
		delete chr;
	}

} // ~Mapper()

u8 Mapper::read8(u16 address)
{
	if (address >= K_32) // PRG ROM
	{
		u16 offsetAddress	= (address - K_32);
		u32 mapVal			= prgMap[offsetAddress / K_8];
		return prg[ mapVal + (offsetAddress % K_8) ];
	}
	else // PRG RAM
	{
		u16 offsetAddress = (address - K_24);
		return prgRAM[offsetAddress];
	}

} // read8()

u8 Mapper::chr_read8(u16 address)
{
	u32 mapAddr = chrMap[address / K_1] + (address % K_1);
	return chr[mapAddr];

} // chr_read8()

// PRG Mapping Function 
void Mapper::MapPRG( int pageSize, int slot, int bank)
{
	// if negative, wrap around
	if (bank < 0)
	{
		u8 numPages = prgSize / (K_1 * pageSize);
		bank += numPages;
	}
	// Populate 8K pages
	for (int i = 0; i < (pageSize / 8); i++)
	{
		u8 curPage = ( (pageSize / 8) * slot) + i;
		prgMap[curPage] = ( (pageSize * K_1 * bank) + (K_8 * i) ) % prgSize;

	} // for

} // MapPRG()

// CHR Mapping Function 
void Mapper::MapCHR(int pageSize, int slot, int bank)
{
	// if negative, wrap around
	if (bank < 0)
	{
		u8 numPages = chrSize / (K_1 * pageSize);
		bank += numPages;
	}
	// Populate 1K Pages
	for (int i = 0; i < pageSize; i++)
	{
		u8 curPage = (pageSize * slot) + i;
		chrMap[curPage] = ( (pageSize * K_1 * bank) + (K_1 * i) ) % chrSize;

	} // for

} // MapCHR()

MAPPER::SaveData Mapper::GrabSaveData()
{ 
	return MAPPER::SaveData( prgRAM, chr, hasChrRAM, prgMap, chrMap );

} // GrabSaveData()

void Mapper::LoadSaveData(MAPPER::SaveData loadedData) 
{ 
	if (hasChrRAM)
	{
		for (int i = 0; i < K_8; i++)
		{
			chr[i] = loadedData.chrRAM[i];
		} // for
	}

	for (int i = 0; i < K_8; i++)
	{
		prgRAM[i] = loadedData.prgRAM[i];
	} // for
	for (int i = 0; i < 4; i++)
	{
		prgMap[i] = loadedData.prgMap[i];
	} // for
	for (int i = 0; i < 8; i++)
	{
		chrMap[i] = loadedData.chrMap[i];
	} // for

} // LoadSaveData()

