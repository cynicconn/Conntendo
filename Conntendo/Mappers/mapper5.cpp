#include "mapper5.h"

// Conntendo
#include "cpu.h"
#include "ppu.h"

#define FETCH_TILE		(~address & 0x3C0)
#define BANK_MODE		(val & 0x03)
#define EXTRA_NAMETABLE (ramExtraMode == 0)
#define EXTRA_ATTRMODE	(ramExtraMode == 1)
#define EXTRA_RAM		(ramExtraMode == 2)
#define EXTRA_RAM_WP	(ramExtraMode == 3)

// Constructor
Mapper5::Mapper5(u8* rom) : Mapper(rom)
{
	// Modes Default to 3 at Startup
	prgBankMode = 3; 
	chrBankMode = 3;

	// NameTable
	nameTableMapping = 0xFF;
	fillModeTile = 0xFF;
	fillModeColor = 0xFF;

	// Split Mode
	verticalSplitMode = 0;
	verticalSplitScroll = 0;
	verticalSplitBank = 0;

	// IRQ
	scanlineCounter = 0;
	triggerScanline = 0;
	irqEnable = false;
	irqPending = false;
	inFrame = false;

	// Multiply
	multiplicand = 0;
	multiplier = 0;

	// Extra RAM
	ramExtraMode = 0;
	ramBankSwitch = 0;
	extraVal = 0;
	attrModeBank = 0;
	ramProtect1 = false;
	ramProtect2 = false;

	// Banks
	upperChrBankBits = 0;
	memset(prgBank, 0xFF, 4 * sizeof(u8));
	memset(sprChrBank, 0xFF, 8 * sizeof(u8));
	memset(bgChrBank, 0xFF, 4 * sizeof(u8));
	memset(bgChrMap, 0, 8 * sizeof(u32));
	memset(isBankROM, 0, 3);

	// RAM
	memset(lgPrgRAM, 0xFF, K_64);
	memset(ntLowerRAM, 0xFF, K_1);
	memset(ntUpperRAM, 0xFF, K_1);
	memset(extraRAM, 0xFF, K_1);

	// Use own NameTable Lookup
	PPU::DisableCIRAM(true);

	SetBanks();

} // Mapper5

void Mapper5::SetBanks()
{
	// PRG Bank Mode
	switch (prgBankMode)
	{
	case 0: // 32K Mode (unused)
		MapPRG(32, 0, prgBank[3] >> 2);
		break;
	case 1: // 16K Mode (unused)
		MapPRG(16, 0, prgBank[1] >> 1);
		MapPRG(16, 1, prgBank[3] >> 1);
		break;
	case 2: // 16K+8K Mode (Castlevania 3)
		MapPRG(16, 0, prgBank[1] >> 1);
		MapPRG(8,  2, prgBank[2]);
		MapPRG(8,  3, prgBank[3]);
		break;
	case 3: // 8K Mode (All other games)
		MapPRG(8, 0, prgBank[0]);
		MapPRG(8, 1, prgBank[1]);
		MapPRG(8, 2, prgBank[2]);
		MapPRG(8, 3, prgBank[3]);
		break;
	} // switch

	// SPR CHR Bank Mode
	switch (chrBankMode)
	{
	case 0: // 8K Mode
		MapCHR(8, 0, sprChrBank[7]);
		break;
	case 1: // 1K Mode
		MapCHR(4, 0, sprChrBank[3]);
		MapCHR(4, 1, sprChrBank[7]);
		break;
	case 2: // 2K Mode
		MapCHR(2, 0, sprChrBank[1]);
		MapCHR(2, 1, sprChrBank[3]);
		MapCHR(2, 2, sprChrBank[5]);
		MapCHR(2, 3, sprChrBank[7]);
		break;
	case 3: // 1K Mode
		for (int i = 0; i < 8; i++)
		{
			MapCHR(1, i, sprChrBank[i]);
		} // for
		break;
	} // switch

	// BG CHR Bank Mode
	switch (chrBankMode)
	{
	case 0: // 8K Mode
		MapChrBG(8, 0, bgChrBank[3]);
		break;
	case 1: // 4K Mode
		MapChrBG(4, 0, bgChrBank[3]);
		MapChrBG(4, 1, bgChrBank[3]);
		break;
	case 2: // 2K Mode
		MapChrBG(2, 0, bgChrBank[1]);
		MapChrBG(2, 1, bgChrBank[3]);
		MapChrBG(2, 2, bgChrBank[1]);
		MapChrBG(2, 3, bgChrBank[3]);
		break;
	case 3: // 1K Mode
		for (int i = 0; i < 8; i++)
		{
			MapChrBG(1, i, bgChrBank[i%4]);
		} // for
		break;
	} // switch

	// Override CHR Bank Switching 
	if (EXTRA_ATTRMODE)
	{
		MapChrBG(4, 0, attrModeBank);
		MapChrBG(4, 1, attrModeBank);
	}

} // SetBanks()

u8 Mapper5::ReadAudio(u16 address)
{
	// TODO
	return 0;
} // ReadAudio()

u8 Mapper5::WriteAudio(u16 address, u8 val)
{
	// TODO
	return val;
} // WriteAudio()

u8 Mapper5::WriteToRAM(u16 address, u8 val)
{
	// RAM is only writable if both flags are set
	if (ramProtect1 && ramProtect2)
	{
		u32 ramAddr = address - K_24;
		ramAddr += K_8 * (ramBankSwitch & 0x07); // RAM Bank Switch
		return lgPrgRAM[ramAddr] = val;
	}
	return 0;

} // WriteToRAM()

u8 Mapper5::write8(u16 address, u8 val)
{ 
	// Write to RAM
	if (address >= 0x6000 && address <= 0x7FFF)
	{
		return WriteToRAM(address, val);
	}
	else if (address >= 0x5000 && address <= 0x5015)
	{
		return WriteAudio(address, val);
	}

	//u8 attrib_bits;
	switch (address)
	{
	case 0x5100: // PRG Bank Mode
		prgBankMode = BANK_MODE;
		break;
	case 0x5101: // CHR Bank Mode
		chrBankMode = BANK_MODE;
		break;
	case 0x5102: // PRG RAM Protect 1
		ramProtect1 = (val & 0x03) == 0x02;
		break;
	case 0x5103: // PRG RAM Protect 2
		ramProtect2 = (val & 0x03) == 0x01;
		break;
	case 0x5104: // Extended RAM Mode
		ramExtraMode = val & 0x03;
		break;
	case 0x5105: // Nametable Mapping
		nameTableMapping = val;
		break;
	case 0x5106: // Fill Mode Tile
		fillModeTile = val;
		break;
	case 0x5107: // Fill Mode Color
		fillModeColor = val;
		break;
	case 0x5113: // PRG RAM Bank Switch
		ramBankSwitch = val & 0x07;
		break;
	case 0x5114: // PRG Bank Mode 0
		prgBank[0] = val & 0x7F;
		isBankROM[0] = val & 0x80;
		break;
	case 0x5115: // PRG Bank Mode 1
		prgBank[1] = val & 0x7F;
		isBankROM[1] = val & 0x80;
		break;
	case 0x5116: // PRG Bank Mode 2
		prgBank[2] = val & 0x7F;
		isBankROM[2] = val & 0x80;
		break;
	case 0x5117: // PRG Bank Mode 3
		prgBank[3] = val & 0x7F;
		break;
	case 0x5200: // Vertical Split Mode
		verticalSplitMode = val;
		break;
	case 0x5201: // Vertical Split Scroll 
		verticalSplitScroll = val;
		break;
	case 0x5202: // Vertical Split Bank
		verticalSplitBank = val;
		break;
	case 0x5203: // IRQ Counter
		triggerScanline = val;
		break;
	case 0x5204: // IRQ Status
		irqEnable = (val & 0x80);
		if (irqEnable && irqPending)
		{
			CPU::Set_IRQ();
		}
		break;
	case 0x5205: // Multiply Instruction
		multiplicand = val;
		break;
	case 0x5206: // Multiply Instruction
		multiplier = val;
		break;
	} // switch

	// CHR Bank Switching
	if (address >= 0x5120 && address <= 0x5127) // Sprite Tiles for 8x16
	{
		u8 slot = address & 0x0F;
		sprChrBank[slot] = val | upperChrBankBits;
	}
	else if (address >= 0x5128 && address <= 0x512B) // Background Tiles for 8x16 (otherwise ignored)
	{
		u8 slot = (address & 0x0F)-8;
		bgChrBank[slot] = val | upperChrBankBits;
	}
	else if (address == 0x5130) // Upper CHR Bank Bits
	{
		upperChrBankBits = (val & 0x03) << 6;
	}

	// Write to Expansion RAM
	if (address >= 0x5C00 && address <= 0x5FFF)
	{
		if (EXTRA_NAMETABLE || EXTRA_ATTRMODE)
		{
			extraRAM[address - 0x5C00] = (inFrame) ? val : 0;
		}
		else if (EXTRA_RAM) // Mode3 means Write Protected
		{
			extraRAM[address - 0x5C00] = val;
		}
	}

	SetBanks();
	return val;

} // write8()

u8 Mapper5::read8(u16 address)
{
	if (address >= 0x5000 && address <= 0x5015)
	{
		return ReadAudio(address);
	}
	else if (address == 0x5204) // Read IRQ Status
	{
		u8 irqStatus = (inFrame << 6) | (irqPending << 7);
		irqPending = false;
		CPU::Clear_IRQ();
		return irqStatus;
	}
	else if (address == 0x5205) // Multiply Insturction Lower
	{
		return (multiplicand * multiplier);
	}
	else if (address == 0x5206) // Multiply Insturction Upper
	{
		return (multiplicand * multiplier) >> 8;
	}
	else if (address >= 0x5C00 && address <= 0x5FFF) // Expansion RAM
	{
		if (EXTRA_RAM || EXTRA_RAM_WP)
		{
			return extraRAM[address - 0x5C00];
		}
	}
	else if (address >= 0x6000 && address <= 0x7FFF) // PRG RAM
	{
		u16 offsetAddress = address - K_24;
		return lgPrgRAM[offsetAddress];
	}
	else // PRG ROM
	{
		u16 offsetAddress = (address - K_32);
		u32 mapVal = prgMap[offsetAddress / K_8];
		return prg[mapVal + (offsetAddress % K_8)];
	}
	return 0;

} // read8()

// BG CHR Mapping Function (Identical except for chrMap)
void Mapper5::MapChrBG(int pageSize, int slot, int bank)
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
		bgChrMap[curPage] = ((pageSize * K_1 * bank) + (K_1 * i)) % chrSize;
	} // for

} // MapChrBG()

u8 Mapper5::chr_read8(u16 address)
{
	u32 mapAddr = (address % K_1);
	u8 slot = address / K_1;

	if (EXTRA_ATTRMODE)
	{
		bool useSprTiles = (PPU::GetCycle() > 320 && PPU::GetCycle() <= 321); // Grab Sprite Pixels occurs on dot 321 (setting == doesnt work)
		mapAddr += (useSprTiles) ? chrMap[slot] : bgChrMap[slot];
	}
	else
	{
		bool useBgTiles = !(PPU::GetCycle() > 257 && PPU::GetCycle() <= 321); // not exactly sure why this range
		mapAddr += (useBgTiles) ? bgChrMap[slot] : chrMap[slot];
	}
	return chr[mapAddr];

} // chr_read8()

u8 Mapper5::chr_write8(u16 address, u8 val)
{
	return chr[address] = val;

} // chr_write8()

// Read Mapper NameTable RAM
u8 Mapper5::ReadExtraRAM(u16 address)
{
	//Extended Attribute Mode
	if (EXTRA_ATTRMODE)
	{
		if (FETCH_TILE)
		{
			u8 coarseX = address & 0x1F;
			u8 coarseY = (address >> 5) & 0x1F;
			extraVal = extraRAM[coarseX + (32 * coarseY)];
			attrModeBank = (extraVal & 0x3F) | upperChrBankBits;
			MapChrBG(4, 0, attrModeBank);
			MapChrBG(4, 1, attrModeBank);
		}
		else // Use the cached value from the previous fetch
		{
			u8 attrVal = extraVal >> 6;
			return (attrVal << 6) | (attrVal << 4) | (attrVal << 2) | attrVal;
		}
	}

	// Vertical Split Mode
	if ( (verticalSplitMode & 0x80) && EXTRA_ATTRMODE)
	{
		// TODO
	}

	u8 table = (address >> 0x09) & 0x06;
	u8 ntMap = (nameTableMapping >> table) & 0x03;
	switch (ntMap)
	{
	case 0: // Screen0
		return ntLowerRAM[address % K_1];
	case 1: // Screen1
		return ntUpperRAM[address % K_1];
	case 2: // Expansion RAM as 3rd NameTable
		if (EXTRA_NAMETABLE || EXTRA_ATTRMODE)
		{
			return extraRAM[address % K_1];
		}
		return 0;
	case 3: // Fill Mode
		return (FETCH_TILE) ? fillModeTile : fillModeColor;
	} // switch

} // ReadExtraRAM()

// Write to Mapper NameTable RAM
u8 Mapper5::WriteExtraRAM(u16 address, u8 val)
{
	u8 table = (address >> 0x09) & 0x06;
	u8 ntMap = (nameTableMapping >> table) & 0x03;
	switch (ntMap)
	{
	case 0: // Screen0
		return ntLowerRAM[address % K_1] = val;
	case 1: // Screen1
		return ntUpperRAM[address % K_1] = val;
	case 2: // Expansion RAM as 3rd NameTable
		if (EXTRA_NAMETABLE || EXTRA_ATTRMODE)
		{
			return extraRAM[address % K_1] = val;
		}
		break;
	} // switch

} // WriteExtraRAM()

// For keeping track if PPU is Rendering
void Mapper5::SignalCPU()
{
	bool isVisibleScanline = (PPU::GetScanline() >= 0 && PPU::GetScanline() <= 239+PPU::GetDebugValue());
	if (!PPU::IsRendering() || !isVisibleScanline)
	{
		inFrame = false;
	}

} // SignalCPU()

// Handle Scanline-Based IRQ Signal
void Mapper5::SignalScanline()
{
	if (!inFrame)
	{
		inFrame = true;
		irqPending = false;
		scanlineCounter = 0;
		CPU::Clear_IRQ();
	}
	else
	{
		scanlineCounter++;
		if (scanlineCounter == triggerScanline)
		{
			irqPending = true;
			if (irqEnable)
			{
				CPU::Set_IRQ();
			}
		}
	}

} // SignalScanline()

MAPPER::SaveData Mapper5::GrabSaveData()
{
	MAPPER::SaveData savedMapper = Mapper::GrabSaveData();

	savedMapper.mapperData[0] = prgBankMode;
	savedMapper.mapperData[1] = chrBankMode;
	savedMapper.mapperData[2] = nameTableMapping;
	savedMapper.mapperData[3] = fillModeTile;
	savedMapper.mapperData[4] = fillModeColor;

	savedMapper.mapperData[5] = verticalSplitMode;
	savedMapper.mapperData[6] = verticalSplitScroll;
	savedMapper.mapperData[7] = verticalSplitBank;

	savedMapper.mapperData[8]  = scanlineCounter;
	savedMapper.mapperData[9]  = triggerScanline;
	savedMapper.mapperData[10] = irqEnable;
	savedMapper.mapperData[11] = irqPending;
	savedMapper.mapperData[12] = inFrame;

	savedMapper.mapperData[13] = multiplicand;
	savedMapper.mapperData[14] = multiplier;

	savedMapper.mapperData[15] = ramExtraMode;
	savedMapper.mapperData[16] = ramBankSwitch;
	savedMapper.mapperData[17] = extraVal;
	savedMapper.mapperData[18] = attrModeBank;
	savedMapper.mapperData[19] = ramProtect1;
	savedMapper.mapperData[20] = ramProtect2;
	savedMapper.mapperData[21] = upperChrBankBits;

	savedMapper.mapperData[22] = prgBank[0];
	savedMapper.mapperData[23] = prgBank[1];
	savedMapper.mapperData[24] = prgBank[2];
	savedMapper.mapperData[25] = prgBank[3];

	savedMapper.mapperData[26] = sprChrBank[0];
	savedMapper.mapperData[27] = sprChrBank[1];
	savedMapper.mapperData[28] = sprChrBank[2];
	savedMapper.mapperData[29] = sprChrBank[3];
	savedMapper.mapperData[30] = sprChrBank[4];
	savedMapper.mapperData[31] = sprChrBank[5];
	savedMapper.mapperData[32] = sprChrBank[6];
	savedMapper.mapperData[33] = sprChrBank[7];

	savedMapper.mapperData[34] = bgChrBank[0];
	savedMapper.mapperData[35] = bgChrBank[1];
	savedMapper.mapperData[36] = bgChrBank[2];
	savedMapper.mapperData[37] = bgChrBank[3];

	int iter = 0;
	for (int i = 0; i < 24; i += 3)
	{
		savedMapper.mapperData[38+i]	= bgChrMap[iter];
		savedMapper.mapperData[38+i+1]	= bgChrMap[iter] >> 8;
		savedMapper.mapperData[38+i+2]	= bgChrMap[iter] >> 16;
		iter++;
	} // for

	// Save NTs and Extra RAM to one pool
	for (int i = 0; i < K_1; i++)
	{
		savedMapper.extraRAM[i]		= ntLowerRAM[i];
		savedMapper.extraRAM[K_1+i] = ntUpperRAM[i];
		savedMapper.extraRAM[K_2+i] = extraRAM[i];
	} // for

	// Only saves first 8K of large RAM
	for (int i = 0; i < K_8; i++)
	{
		savedMapper.prgRAM[i] = lgPrgRAM[i];
	} // for

	return savedMapper;

} // GrabSaveData()

void Mapper5::LoadSaveData(MAPPER::SaveData loadedData)
{
	Mapper::LoadSaveData(loadedData);

	prgBankMode			= loadedData.mapperData[0];
	chrBankMode			= loadedData.mapperData[1];
	nameTableMapping	= loadedData.mapperData[2];
	fillModeTile		= loadedData.mapperData[3];
	fillModeColor		= loadedData.mapperData[4];

	verticalSplitMode	= loadedData.mapperData[5];
	verticalSplitScroll = loadedData.mapperData[6];
	verticalSplitBank	= loadedData.mapperData[7];

	scanlineCounter		= loadedData.mapperData[8];
	triggerScanline		= loadedData.mapperData[9];
	irqEnable			= loadedData.mapperData[10];
	irqPending			= loadedData.mapperData[11];
	inFrame				= loadedData.mapperData[12];

	multiplicand		= loadedData.mapperData[13];
	multiplier			= loadedData.mapperData[14];

	ramExtraMode		= loadedData.mapperData[15];
	ramBankSwitch		= loadedData.mapperData[16];
	extraVal			= loadedData.mapperData[17];
	attrModeBank		= loadedData.mapperData[18];
	ramProtect1			= loadedData.mapperData[19];
	ramProtect2			= loadedData.mapperData[20];
	upperChrBankBits	= loadedData.mapperData[21];

	prgBank[0]			= loadedData.mapperData[22];
	prgBank[1]			= loadedData.mapperData[23];
	prgBank[2]			= loadedData.mapperData[24];
	prgBank[3]			= loadedData.mapperData[25];

	sprChrBank[0] = loadedData.mapperData[26];
	sprChrBank[1] = loadedData.mapperData[27];
	sprChrBank[2] = loadedData.mapperData[28];
	sprChrBank[3] = loadedData.mapperData[29];
	sprChrBank[4] = loadedData.mapperData[30];
	sprChrBank[5] = loadedData.mapperData[31];
	sprChrBank[6] = loadedData.mapperData[32];
	sprChrBank[7] = loadedData.mapperData[33];

	bgChrBank[0] = loadedData.mapperData[34];
	bgChrBank[1] = loadedData.mapperData[35];
	bgChrBank[2] = loadedData.mapperData[36];
	bgChrBank[3] = loadedData.mapperData[37];

	//memset(bgChrMap, 0, 8 * sizeof(u32));
	int iter = 0;
	for (int i = 0; i < 24; i += 3)
	{
		bgChrMap[iter] = (loadedData.mapperData[38+i] << 0) | (loadedData.mapperData[38+i+1] << 8) | (loadedData.mapperData[38+i+2] << 16);
		iter++;
	} // for

	for (int i = 0; i < K_1; i++)
	{
		ntLowerRAM[i] = loadedData.extraRAM[i];
		ntUpperRAM[i] = loadedData.extraRAM[K_1+i];
		extraRAM[i] = loadedData.extraRAM[K_2+i];
	} // for

	// only first 8K is loaded into MMC5's large RAM pool
	for (int i = 0; i < K_8; i++)
	{
		lgPrgRAM[i] = loadedData.prgRAM[i];
	} // for

	SetBanks();

} // LoadSaveData()
