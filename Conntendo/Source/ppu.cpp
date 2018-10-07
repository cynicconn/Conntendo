#include "ppu.h"

// Conntendo
#include "cpu.h"
#include "cartridge.h"
#include "emulator.h"
#include "palette.h"
#include "viewer.h"

namespace PPU
{
// Debug Consts
#define UPDATE_DEBUG_RATE 30

// Helper Macros
#define IS_UNUSED_PALETTE_LOC	(address & 0x13) == 0x10
#define IS_VISIBLE_AREA			(scanline < HEIGHT && xPos >= 0 && xPos < WIDTH)
#define IS_SPRITE_ENABLED		IS_SET(mask, PPU_MASK::SPR_ENABLE)
#define IS_BACKGROUND_ENABLED	IS_SET(mask, PPU_MASK::BKD_ENABLE)
#define IS_SPRITE_LEFTCOL		(IS_SET(mask, PPU_MASK::SPR_LEFTCOL_ENABLE) || xPos >= 8)
#define IS_BACKGROUND_LEFTCOL	(IS_SET(mask, PPU_MASK::BKD_LEFTCOL_ENABLE) || xPos >= 8)
#define BG_SHIFT_REGISTER		(ppuCycle >= 2 && ppuCycle <= 255) || (ppuCycle >= 322 && ppuCycle <= 337)

// Hex Modifers
#define HOR_UPDATE  0x041F
#define VERT_UPDATE 0x7BE0

// PPU Registers (0x200x)
#define PPUCTRL		0
#define PPUMASK		1
#define PPUSTATUS	2
#define OAMADDR		3
#define OAMDATA		4
#define PPUSCROLL	5
#define PPUADDR		6
#define PPUDATA		7

// Cycle and Scanline Markers
#define CYCLE_END				340
#define SCANLINE_PRE			-1
#define SCANLINE_VISIBLE_END	239
#define SCANLINE_POST			240
#define SCANLINE_NMI			241
#define SCANLINE_NMI_END		260
#define SCANLINE_END			261

	// NameTable Variables
	Mirroring mirrorMode;				// Nametable Mirroring Mode
	bool ciRAMDisabled = false;			// Disable ciRAM, use cartridge RAM instead ( for gauntlet and rad racer etc )

	// vRAM Variables
	u8 ciRAM[K_2];						// VRAM for nametables ( enough for two )
	u8 cgRAM[32];						// VRAM for palettes
	u8 oamMem[256];						// VRAM for sprite properties ( Object Attribute Memory )

	// Sprite Memory
	Sprite oam[SPRITE_LIMIT];			// Sprite Buffer
	Sprite secOAM[SPRITE_LIMIT];		// Secondary Sprite Buffers 

	// Screen Buffer
	u32 pixelBuffer[WIDTH * HEIGHT];	// Screen Buffer 256x240

	// vRAM Address
	PPU_ADDRESS vRamAddr;
	PPU_ADDRESS tempAddr;
	u8 fineX;
	u8 oamAddress;

	// Rendering Counters
	int scanline;
	int ppuCycle; 
	bool isEvenFrame;

	// PPU Flags
	u8 ctrl;
	u8 mask;
	u8 status;

	// Background Latches ( for fetching tile data, every 8 cycles )
	u8 nameTable;
	u8 attrTable;
	u8 bgLow;
	u8 bgHigh; // (+8 bytes from tile bgLow )

	// Background Shift Registers
	u8 attrShiftLow;
	u8 attrShiftHigh;
	u16 bgShiftLow;
	u16 bgShiftHigh;
	bool attrLatchLow;
	bool attrLatchHigh;

	// PPU Temp Storage
	static u16	renderAddress;	// address to character tables
	static u8	memRes;			// Result of the operation
	static u8	memBuffer;		// VRAM read buffer
	static bool	memLatch;		// Detect second reading

	// Dev Debug
	int debugIncr = 0;
	int debugCounter = 0;

	// Debug Display Options
	bool bDebugHighlightSprites		= false;
	bool bDebugFillSprites			= false; 
	bool bDebugDisableBackground	= false;
	bool bDebugNametableViewer		= false;
	bool bDebugPatternTableViewer	= false;
	bool bDebugDisableSpriteOffsetX = false;  
	bool bDebugDisableSpriteOffsetY = false;

	// Debug Count PPU Cycles
	int GetCycle() { return ppuCycle; }
	int GetScanline() { if (scanline <= 0 || scanline >= SCANLINE_END) return 0;  return scanline; }

	//-------------- Toggle Emulator Settings --------------//

	// when true, disable Alpha pixels
	bool ToggleFillSprites()
	{
		return bDebugFillSprites = !bDebugFillSprites;

	} // ToggleFillSprites()

	// when true, sprite is one color
	bool ToggleHighlightSprites()
	{
		return bDebugHighlightSprites = !bDebugHighlightSprites;

	} // ToggleHighlightSprites()

	bool ToggleDisableBackground()
	{
		return bDebugDisableBackground = !bDebugDisableBackground;

	} // ToggleDisableBackground()

	bool ToggleDisableSpriteOffset()
	{
		if (bDebugDisableSpriteOffsetX)
		{
			bDebugDisableSpriteOffsetX = false;
			bDebugDisableSpriteOffsetY = false;
		}
		else
		{
			bDebugDisableSpriteOffsetX = true;
			bDebugDisableSpriteOffsetY = true;
		}
		return bDebugDisableSpriteOffsetX;

	} // ToggleDisableSpriteOffset()

	void ToggleNametableViewer(bool toView)
	{
		bDebugNametableViewer = toView;

	} // ToggleNametableViewer()

	void TogglePatternTableViewer(bool toView)
	{
		bDebugPatternTableViewer = toView;

	} // TogglePatternTableViewer()

	int GetDebugValue() 
	{ 
		return debugIncr;

	} // GetDebugValue() 

	void IncrementDebugValue(bool incr)
	{
		debugIncr = (incr) ? debugIncr + 1 : debugIncr - 1;
		string verMessage = "Debug Val: " + std::to_string(PPU::GetDebugValue());
		Emulator::ShowMessage(verMessage);

	} // IncrementDebugValue()

	//------------------------------------------//

	PPU::SaveData GrabSaveData()
	{
		PPU::SaveData ppuData(mirrorMode, ciRAM, cgRAM, oamMem, oam, secOAM);

		ppuData.ctrl	= ctrl;
		ppuData.mask	= mask;
		ppuData.status	= status;

		ppuData.scanline	= scanline;
		ppuData.cycle		= ppuCycle;
		ppuData.isEvenFrame	= isEvenFrame;

		ppuData.vRamAddr	= vRamAddr;
		ppuData.tempAddr	= tempAddr;
		ppuData.fineX		= fineX;
		ppuData.oamAddress	= oamAddress;

		ppuData.nameTable	= nameTable;
		ppuData.attrTable	= attrTable;
		ppuData.bgLow		= bgLow;
		ppuData.bgHigh		= bgHigh;

		ppuData.attrShiftLow	= attrShiftLow;
		ppuData.attrShiftHigh	= attrShiftHigh;
		ppuData.bgShiftLow		= bgShiftLow;
		ppuData.bgShiftHigh		= bgShiftHigh;
		ppuData.attrLatchLow	= attrLatchLow;
		ppuData.attrLatchHigh	= attrLatchHigh;

		ppuData.renderAddress	= renderAddress;
		ppuData.memRes			= memRes;
		ppuData.memBuffer		= memBuffer;
		ppuData.memLatch		= memLatch;

		return ppuData;

	} // GrabSaveData()

	void LoadSaveData(PPU::SaveData loadedData)
	{
		mirrorMode	= loadedData.mirrorMode;

		ctrl		= loadedData.ctrl;
		mask		= loadedData.mask;
		status		= loadedData.status;

		scanline	= loadedData.scanline;
		ppuCycle	= loadedData.cycle;
		isEvenFrame = loadedData.isEvenFrame;

		vRamAddr	= loadedData.vRamAddr;
		tempAddr	= loadedData.tempAddr;
		fineX		= loadedData.fineX;
		oamAddress	= loadedData.oamAddress;

		nameTable	= loadedData.nameTable;
		attrTable	= loadedData.attrTable;
		bgLow		= loadedData.bgLow;
		bgHigh		= loadedData.bgHigh;

		attrShiftLow	= loadedData.attrShiftLow;
		attrShiftHigh	= loadedData.attrShiftHigh;
		bgShiftLow		= loadedData.bgShiftLow;
		bgShiftHigh		= loadedData.bgShiftHigh;
		attrLatchLow	= loadedData.attrLatchLow;
		attrLatchHigh	= loadedData.attrLatchHigh;

		renderAddress	= loadedData.renderAddress;
		memRes			= loadedData.memRes;
		memBuffer		= loadedData.memBuffer;
		memLatch		= loadedData.memLatch;

		memcpy(ciRAM, loadedData.ciRAM, sizeof(loadedData.ciRAM));
		memcpy(cgRAM, loadedData.cgRAM, sizeof(loadedData.cgRAM));
		memcpy(oamMem, loadedData.oamMem, sizeof(loadedData.oamMem));
		memcpy(oam, loadedData.oam, sizeof(loadedData.oam));
		memcpy(secOAM, loadedData.secOAM, sizeof(loadedData.secOAM));

	} // LoadSaveData()

	//Get CIRAM address according to Mirroring
	u16 GetNameTable(u16 address)
	{
		switch (mirrorMode)
		{
		case VERTICAL:
			return (address % K_2);
		case HORIZONTAL:
			return ((address / 2) & K_1) + (address % K_1);
		case ONESCREEN_LOW:
			return (address % K_1);
		case ONESCREEN_UP:
			return ((address % K_1) + K_1) % K_2; // Holy Cow! This was causing RC-ProAm to touch other parts of code.....corrupt the Emulator :|
		} // switch 
		Emulator::ShowMessage("Invalid Mirror Mode"); // TODO: assert
		return 0;

	} // GetNameTable()

	// use Cartridge vRAM for Nametables instead of normal PPU ( Gauntlet )
	void DisableCIRAM(bool toDisable) { ciRAMDisabled = toDisable; }

	void SetMirrorMode(Mirroring newMode)
	{
		mirrorMode = newMode;

	} // SetMirrorMode() 

	// Return which AddressSpace is being accessed in PPU
	PPU_MEMMAP GetMapLoc(u16 address)
	{
		if (address <= MEMMAP_PT_END)
		{
			return PPU_MEMMAP::CHR;
		}
		else if (address >= MEMMAP_NT && address <= MEMMAP_NT_END)
		{
			return PPU_MEMMAP::Nametable;
		}
		else if (address >= MEMMAP_PALETTE && address <= MEMMAP_PALETTE_END)
		{
			return PPU_MEMMAP::Palette;
		}
		return PPU_MEMMAP::None;

	} // GetMapLoc()

	// Write to 8-Bit Address
	inline u8  write8(u16 address, u8 val)
	{
		switch (GetMapLoc(address))
		{
		case PPU_MEMMAP::CHR:
			return Cartridge::WriteCHR(address, val);
			break;
		case PPU_MEMMAP::Nametable:
			if (ciRAMDisabled)
			{
				Cartridge::WriteExtraRAM(address, val);
			}
			return ciRAM[GetNameTable(address)] = val;
			break;
		case PPU_MEMMAP::Palette:
			if (IS_UNUSED_PALETTE_LOC)
			{
				CLEAR_BIT(address, 0x10); // force to choose background palette
			}
			cgRAM[address & 0x1F] = val;
			break;
		} // switch

	} // write8()

	inline u8 GetPalette(u16 address)
	{
		u8 isGreyscale = IS_SET(mask, PPU_MASK::GREYSCALE) ? 0x30 : 0xFF;
		return cgRAM[address & 0x1F] & isGreyscale;

	} // GetPalette()

	inline u8 ReadNameTable(u16 address)
	{
		if (ciRAMDisabled)
		{
			return Cartridge::ReadExtraRAM(address);
		}
		return ciRAM[GetNameTable(address)];

	} // ReadNameTable()

	// Read and return 8-Bit Value
	inline u8 read8(u16 address)
	{
		switch (GetMapLoc(address))
		{
		case PPU_MEMMAP::CHR:
			return Cartridge::ReadCHR(address);
		case PPU_MEMMAP::Nametable:
			return ReadNameTable(address);
		case PPU_MEMMAP::Palette:
			return GetPalette(address);
		case PPU_MEMMAP::None:
			return 0;
		} // switch

	} // read8()

	// When writing PPUCTRL
	inline void WriteCtrl(u16 address, u8 val)
	{
		if (!(ctrl & PPU_CTRL::NMI_ENABLED) && (val & PPU_CTRL::NMI_ENABLED) && (status & PPU_STATUS::VBLANK))
		{
			CPU::Set_NMI(); // Set NMI right away if vBlank set (but not repeatedly)
		}
		ctrl = val;
		tempAddr.nameTable = IS_SET(ctrl, PPU_CTRL::NAMETABLE_SLCT_A) | IS_SET(ctrl, PPU_CTRL::NAMETABLE_SLCT_B); // get lowest two bytes of PPUCTRL 

	} // WriteCtrl()

	// When writing PPUSCROLL
	inline void WriteScroll(u16 address, u8 val)
	{
		if (!memLatch) // First Write X-Scroll
		{
			fineX = MASK_OFF(val, 0x07);
			tempAddr.coarseX = val >> 3;
		}
		else // Second Write Y-Scroll
		{
			tempAddr.fineY = MASK_OFF(val, 0x07);
			tempAddr.coarseY = val >> 3;
		}
		memLatch = !memLatch;

	} // WriteScroll()

	// When writing PPUADDR
	inline void WriteAddr(u16 address, u8 val)
	{
		if (!memLatch) // First Write
		{
			tempAddr.high = val & 0x003F;
		}
		else // Second Write
		{
			tempAddr.low = val;
			vRamAddr.reg = tempAddr.reg;
		}
		memLatch = !memLatch;

	} // WriteAddr()

	// When writing PPUADDR
	inline void WriteData(u16 address, u8 val)
	{
		write8(vRamAddr.address, val);
		vRamAddr.address += IS_SET(ctrl, PPU_CTRL::INCREMENT_MODE) ? 32 : 1; // Fine or Coarse 

	} // WriteData()

	u8 WriteMemory(u16 address, u8 val)
	{
		u16 index = address % 8;
		memRes = val;

		switch (index)
		{
		case PPUCTRL:
			WriteCtrl(address, val);
			break;
		case PPUMASK:
			mask = val;
			break;
		case OAMADDR:
			oamAddress = val;
			break;
		case OAMDATA:
			oamMem[oamAddress++] = val;
			break;
		case PPUSCROLL:
			WriteScroll(address, val);
			break;
		case PPUADDR:
			WriteAddr(address, val);
			break;
		case PPUDATA:
			WriteData(address, val);
		} // switch

		return memRes;

	} // WriteMemory()

	// When reading PPUSTATUS
	inline void ReadStatus(u16 address)
	{
		memRes = (memRes & 0x1F) | status; // nesdev says lower 5 bits are runoff from last PPU memory read
		//Caution: Reading PPUSTATUS at the exact start of vertical blank will return 0 in bit 7 but clear the latch anyway, causing the program to miss frames
		if (scanline == SCANLINE_NMI && ppuCycle <= 1) // Start of VBlank
		{
			memRes &= 0x7F;
		}
		// clear VBlank Status when reading PPUSTATUS   
		CLEAR_BIT(status, PPU_STATUS::VBLANK); // Not clearing this was causing KungFu, SuperMario to Freeze  ( ! instead of ~ )
		memLatch = 0;

	} // ReadStatus()

	// When reading PPUDATA
	inline void ReadData(u16 address)
	{
		if (vRamAddr.address <= 0x3EFF)
		{
			memRes = memBuffer;
			memBuffer = read8(vRamAddr.address);
		}
		else
		{
			memRes = read8(vRamAddr.address);
			memBuffer = memRes;
		}
		vRamAddr.address += IS_SET(ctrl, PPU_CTRL::INCREMENT_MODE) ? 32 : 1;

	} // ReadData()

	u8 ReadMemory(u16 address)
	{
		u16 index = address % 8;
		switch (index)
		{
		case PPUSTATUS:
			ReadStatus(address);
			break;
		case OAMDATA:
			memRes = oamMem[oamAddress];
			break;
		case PPUDATA:
			ReadData(address);
			break;
		} // switch
		return memRes;

	} // ReadMemory()

	u8 DebugReadMemory(u16 address)
	{
		if (address == 0x2000)
		{
			return ctrl;
		}
		return read8(address);

	} // DebugReadMemory()

	// Return true if either background or sprites are rendering
	inline bool EitherRendering()
	{
		return IS_SET(mask, PPU_MASK::BKD_ENABLE) || IS_SET(mask, PPU_MASK::SPR_ENABLE);

	} // EitherRendering()

	// for Mappers
	bool IsRendering()
	{
		return EitherRendering();

	} // IsRendering()

	// Return true only if BOTH background and sprites are rendering
	inline bool BothRendering()
	{
		return IS_SET(mask, PPU_MASK::BKD_ENABLE) && IS_SET(mask, PPU_MASK::SPR_ENABLE);

	} // BothRendering()

	// Sprites are either 8x8 or 8x16
	inline int SpriteHeight()
	{
		return IS_SET(ctrl, PPU_CTRL::SPR_HEIGHT) ? 16 : 8;

	} // SpriteHeight()

	// Process one pixel, draw if its onscreen
	void ProcessPixel()
	{
		// Pixel Data
		u8 palette			= 0;
		u8 objPalette		= 0;
		u16 xPos			= ppuCycle - 2;
		bool objPriority	= false; // if true, Sprites draw in front

		// Debug Flags
		bool toDebugAlpha = false;
		bool toDebugHighlight = false;

		// Only Draw Pixel in Visible Area
		if (IS_VISIBLE_AREA)
		{
			// Background	
			if (IS_BACKGROUND_ENABLED && IS_BACKGROUND_LEFTCOL)
			{
				palette = (NTH_BIT(bgShiftHigh, 15 - fineX) << 1) | NTH_BIT(bgShiftLow, 15 - fineX); // Get TileMap Data
				if (palette)
				{
					palette |= ((NTH_BIT(attrShiftHigh, 7 - fineX) << 1) | NTH_BIT(attrShiftLow, 7 - fineX)) << 2; // Get Correct Color Palette
				}
			}

			// Sprites
			if (IS_SPRITE_ENABLED && IS_SPRITE_LEFTCOL)
			{
				for (int i = SPRITE_LIMIT - 1; i >= 0; i--)
				{
					if (oam[i].index == 64)
					{
						continue;  // Void entry
					}

					u16 sprX = xPos - oam[i].posX;
					if (sprX >= 8)
					{
						continue; // Not in range
					}
					if (oam[i].attr & 0x40)
					{
						sprX ^= 0x07; // Horizontal Flip
					}

					u8 xOffset = 7 - sprX;
					if (bDebugDisableSpriteOffsetX)
					{
						xOffset = (oam[i].attr & 0x40) ?  xPos % 8 : (255 - xPos) % 8;
					} 
					u8 sprPalette = (NTH_BIT(oam[i].dataH, xOffset) << 1) | NTH_BIT(oam[i].dataL, xOffset);

					// Zero is Transparent Pixel, Dont Draw
					if (sprPalette == 0)
					{
						if (bDebugFillSprites)
						{
							toDebugAlpha = true;
						}
						continue;
					}
					if (bDebugHighlightSprites)
					{
						toDebugHighlight = true;
					}

					// Check for SpriteZeroHit: when an opaque pixel of sprite 0 overlaps an opaque pixel of the background
					if (oam[i].index == 0 && palette != 0 && xPos != 0xFF && IS_BACKGROUND_ENABLED )
					{
						SET_BIT(status, PPU_STATUS::SPR_ZERO_HIT);
					}

					// Grab Sprite Attribute
					sprPalette |= (oam[i].attr & 0x03) << 2;
					objPalette  = sprPalette + 0x10;
					objPriority = oam[i].attr & 0x20;

				} // for
			}

			// Debug Disable Background
			if (bDebugDisableBackground)
			{
				palette = 0x0;
			}

			// Whether to draw Sprite in front of Background
			if ( objPalette && (palette == 0 || objPriority == 0) )
			{
				palette = objPalette;
			}

			u8 thePalette = EitherRendering() ? palette : 0;
			u8 colorIndex = read8(MEMMAP_PALETTE + thePalette);

			// Use debug color instead of normal
			if (toDebugAlpha)
			{
				colorIndex = PALETTE_MAGENTA;
			}

			// Write Color value to current pixel
			int currentPixel			= (scanline * 256) + xPos;
			u32 finalColor				= (toDebugHighlight) ? COLOR_DEBUG_HEX : Palette::GetColor(colorIndex);
			pixelBuffer[currentPixel]	= finalColor;
		}

		// Perform Background Shifts
		bgShiftLow		<<= 1;
		bgShiftHigh		<<= 1;
		attrShiftLow	= (attrShiftLow << 1)	| (u8)attrLatchLow;
		attrShiftHigh	= (attrShiftHigh << 1)	| (u8)attrLatchHigh;

	} // ProcessPixel()

	  // Copy scrolling data from TempAddress and vRAMAddress
	inline void HorUpdate()
	{
		if ( !EitherRendering() )
		{
			return;
		}
		vRamAddr.reg = (vRamAddr.reg & ~HOR_UPDATE) | (tempAddr.reg & HOR_UPDATE);

	} // HorUpdate() 

	inline void VertUpdate()
	{
		if ( !EitherRendering() )
		{
			return;
		}
		vRamAddr.reg = (vRamAddr.reg & ~VERT_UPDATE) | (tempAddr.reg & VERT_UPDATE);

	} // VertUpdate()

	inline void ReloadShift()
	{
		// Load next Tilemap
		bgShiftLow  = (bgShiftLow & 0xFF00)  | bgLow;
		bgShiftHigh = (bgShiftHigh & 0xFF00) | bgHigh;

		// Load next Palette
		attrLatchLow  = (attrTable & 0x01);
		attrLatchHigh = (attrTable & 0x02);

	} // ReloadShift()

	// Calculate Graphics Addresses 
	inline u16 NameTableAddress()
	{
		return K_8 | (vRamAddr.reg & 0x0FFF); // Fetch a nametable entry from $2000-$2FBF

	} // NameTableAddress()

	// Grab Attribute Table
	inline u16 AttrTableAddress()
	{
		return MEMMAP_AT | (vRamAddr.nameTable << 10) | ((vRamAddr.coarseY / 4) << 3) | (vRamAddr.coarseX / 4);

	} // AttrTableAddress()

	// Either first or second Table
	inline u16 BitMapAddress()
	{
		u16 bgTableAddress = IS_SET(ctrl, PPU_CTRL::BKD_TILE_SLCT) ? K_4 : 0;
		return bgTableAddress + (nameTable * 16) + vRamAddr.fineY;

	} // BitMapAddress()

	inline void HorScroll()
	{
		if (!EitherRendering())
		{
			return;
		}

		// Increment Coarse
		if (vRamAddr.coarseX == 0x1F)
		{
			vRamAddr.reg ^= HOR_UPDATE;
		}
		else
		{
			vRamAddr.coarseX++;
		}

	} // HorScroll()

	inline void VertScroll()
	{
		if (!EitherRendering())
		{
			return;
		}

		// Increment either Fine or Coarse
		if (vRamAddr.fineY < 7)
		{
			vRamAddr.fineY++;
		}
		else 
		{
			vRamAddr.fineY = 0;
			if (vRamAddr.coarseY == 0x1F)
			{
				vRamAddr.coarseY = 0;
			}
			else if (vRamAddr.coarseY == 0x1D)
			{
				vRamAddr.coarseY	= 0;
				vRamAddr.nameTable ^= 0x02;
			}
			else
			{
				vRamAddr.coarseY++;
			}
		}

	} // VertScroll()

	// Load the sprite info into main OAM and fetch their tile data for the current scanline
	void GrabSpritePixels()
	{
		u16 address;

		// grab specific row of pixels (based on yPos) for each sprite on scanline
		for (int i = 0; i < SPRITE_LIMIT; i++)
		{
			oam[i] = secOAM[i];  // Copy secondary OAM into primary

			// Address Mode based on Sprite Height
			if (SpriteHeight() == 16)
			{
				address = ((oam[i].tileID & 1) * K_4) + ((oam[i].tileID & 0xFFFE) * 16);
			}
			else // Height 8
			{
				u16 patternTableAddress = IS_SET(ctrl, PPU_CTRL::SPR_TILE_SLCT) ? K_4 : 0;
				address = patternTableAddress + (oam[i].tileID * 16);
			}

			u8 yOffset = (bDebugDisableSpriteOffsetY) ? 0 : oam[i].posY;
			u8 sprY = (scanline - yOffset) % SpriteHeight();  // specific row of pixels based on yOffset, scanline
			if ( IS_SET(oam[i].attr, 0x80) )
			{
				sprY ^= SpriteHeight() - 1;	// Vertical Flip
			}
			address += sprY + (sprY & 8); // Select the second tile if on 8x16

			// Grab Low and High Data
			oam[i].dataL = read8(address + 0);
			oam[i].dataH = read8(address + 8);

		} // for

	} // GrabSpritePixels()

	// Fill secondary OAM with the Sprite Info for the NEXT scanline
	void EvaluateSprites()
	{
		int n = 0;
		for (int i = 0; i < SPRITE_TOTAL; i++)
		{
			int line = (scanline == SCANLINE_END ? -1 : scanline) - oamMem[i * 4 + 0]; // scanline - posY

			// If sprite is in the scanline, copy its properties into secondary OAM
			if (line >= 0 && line < SpriteHeight())
			{
				secOAM[n].index		= i;
				secOAM[n].posY		= oamMem[i * 4 + 0];
				secOAM[n].tileID	= oamMem[i * 4 + 1];
				secOAM[n].attr		= oamMem[i * 4 + 2];
				secOAM[n].posX		= oamMem[i * 4 + 3];
				n++;

				// Set Overflow Flag if more than N sprites on one scanline... 
				if (n >= SPRITE_LIMIT)
				{
					SET_BIT(status, PPU_STATUS::SPR_OVER_EIGHT); 
					break;
				}
			}
		} // for

	} // EvaluateSprites()

	// Clear Secondary OAM of sprite data
	void ClearOAM()
	{
		// Fill with default data
		for (int i = 0; i < SPRITE_LIMIT; i++)
		{
			secOAM[i].index		= 0x40;
			secOAM[i].tileID	= 0xFF;

			secOAM[i].posX		= 0xFF;
			secOAM[i].posY		= 0xFF;

			secOAM[i].attr		= 0xFF;

			secOAM[i].dataL		= 0;
			secOAM[i].dataH		= 0;

		} // for

	} // ClearOAM()

	inline void GrabNameTable( bool grabAddress)
	{
		if (grabAddress)
		{
			renderAddress = NameTableAddress();
			ReloadShift();
		}
		else
		{
			nameTable = read8(renderAddress);
		}

	} // GrabNameTable()

	inline void GrabAttributeTable(bool grabAddress)
	{
		if (grabAddress)
		{
			renderAddress = AttrTableAddress();
		}
		else
		{
			attrTable = read8(renderAddress);
			if (vRamAddr.coarseY & 2)
			{
				attrTable >>= 4;
			}
			if (vRamAddr.coarseX & 2)
			{
				attrTable >>= 2;
			}
		}

	} // GrabAttributeTable()

	inline void GrabBGLowLatch(bool grabAddress)
	{
		if (grabAddress)
		{
			renderAddress = BitMapAddress();
		}
		else
		{
			bgLow = read8(renderAddress);
		}

	} // GrabBGLowLatch()

	inline void GrabBGHighLatch(bool grabAddress)
	{
		if (grabAddress)
		{
			renderAddress += 0x08;
		}
		else
		{
			bgHigh = read8(renderAddress);
			HorScroll();
		}

	} // GrabBGHighLatch()

	inline void CheckEndCycle(Scanline scan)
	{
		// on Odd frame, skip one clock, unless rendering is disabled
		int  toSkipClock = (!isEvenFrame && EitherRendering() && scan == PRE) ? 1 : 0; 
		if (ppuCycle >= (CYCLE_END - toSkipClock))
		{
			if (toSkipClock)
			{
				ppuCycle++;
			}
			else
			{
				nameTable = read8(renderAddress);
			}
		}

	} // CheckEndCycle()

	void VisibleScanline(Scanline scan)
	{
		// Sprites
		switch (ppuCycle)
		{
		case 1:
			ClearOAM();
			if (scan == PRE)
			{
				CLEAR_BIT(status, PPU_STATUS::SPR_OVER_EIGHT);
				CLEAR_BIT(status, PPU_STATUS::SPR_ZERO_HIT);
				CLEAR_BIT(status, PPU_STATUS::VBLANK);
			}
			break;
		case 257:
			EvaluateSprites();
			break;
		case 321:
			GrabSpritePixels();
			break;
		} // switch

		// Background
		if (BG_SHIFT_REGISTER)
		{
			ProcessPixel();
			switch (ppuCycle % 8)
			{
			case 1:
				GrabNameTable(true);
				break;
			case 2:
				GrabNameTable(false);
				break;
			case 3:
				GrabAttributeTable(true);
				break;
			case 4:
				GrabAttributeTable(false);
				break;
			case 5:
				GrabBGLowLatch(true);
				break;
			case 6:
				GrabBGLowLatch(false);
				break;
			case 7:
				GrabBGHighLatch(true);
				break;
			case 0:
				GrabBGHighLatch(false);
				break;
			} // switch()
		}
		else
		{
			if (ppuCycle == 256) // Vertical Bump
			{
				ProcessPixel();
				bgHigh = read8(renderAddress);
				VertScroll();
			}
			else if (ppuCycle == 257) // Update Horizontal Position
			{
				ProcessPixel();
				ReloadShift();
				HorUpdate();
			}
			else if (ppuCycle >= 280 && ppuCycle <= 304) // Update Vertical Position
			{
				if (scan == Scanline::PRE)
				{
					VertUpdate();
				}
			}
			else if (ppuCycle == 1) // No Shift Reloading
			{
				renderAddress = NameTableAddress();
				if (scan == Scanline::PRE)
				{
					CLEAR_BIT(status, PPU_STATUS::VBLANK);
				}
			}
			else if (ppuCycle == 321 || ppuCycle == 339) // No Shift Reloading
			{
				renderAddress = NameTableAddress();
			}
			else if (ppuCycle == 338) // Nametable fetch instead of attribute:
			{
				nameTable = read8(renderAddress);
			}
			CheckEndCycle(scan);
		}

		// IRQ Signal to Mapper based on Scanline
		if (ppuCycle == 260 && EitherRendering())
		{
			Cartridge::SignalScanline();
		}

	} // VisibleScanline()

	// Draw DebugViewers
	void DrawDebugFrame()
	{
		if (debugCounter >= UPDATE_DEBUG_RATE)
		{
			debugCounter = 0;
			if (bDebugNametableViewer)
			{
				Viewer::DrawNametableViewer();
			}
			if (bDebugPatternTableViewer)
			{
				Viewer::DrawPatternTableViewer();
			}
		}
		debugCounter++;

	} // DrawDebugFrame()

	Scanline GetScanlineType()
	{
		if (scanline == SCANLINE_PRE) // 261 should never be hit 
		{
			return Scanline::PRE;
		}
		else if (scanline >= 0 && scanline <= SCANLINE_VISIBLE_END)
		{
			return Scanline::VISIBLE;
		}
		else if (scanline == SCANLINE_POST)
		{
			return Scanline::POST;
		}
		else if (scanline >= SCANLINE_NMI && scanline <= SCANLINE_NMI_END)
		{
			return Scanline::NMI;
		}
		return Scanline::Invalid;
		{
			Emulator::ShowMessage("Error, Invalid Scanline Hit"); // TODO: assert
		}

	} // GetScanlineType()

	// Execute a Cycle of a Scanline
	void ScanlineCycle(Scanline scan)
	{
		if (scan == Scanline::NMI && ppuCycle == 1 && scanline == SCANLINE_NMI) // Set VBlank at (2nd ppuCycle) of scanline 241
		{
			SET_BIT(status, PPU_STATUS::VBLANK);
			if (IS_SET(ctrl, PPU_CTRL::NMI_ENABLED))
			{
				CPU::Set_NMI();
			}
		}
		else if (scan == Scanline::POST && ppuCycle == 0)
		{
			DrawDebugFrame();
			Emulator::NewFrame(pixelBuffer);
		}
		else if (scan == Scanline::VISIBLE || scan == Scanline::PRE)
		{
			VisibleScanline(scan);
		}

	} // ScanlineCycle()

	// Run one PPU Cycle
	void Execute()
	{
		// Run Scanline
		const Scanline type = GetScanlineType();
		ScanlineCycle(type);

		// Update ppuCycle and Scanline Counters
		ppuCycle++;
		if (ppuCycle > CYCLE_END)
		{
			ppuCycle = 0;
			scanline++;
		}
		if (scanline >= SCANLINE_END)
		{
			scanline	= SCANLINE_PRE; 
			isEvenFrame = !isEvenFrame;
		}

	} // Execute()

	void Reset()
	{
		// Reset Counters
		isEvenFrame	= true;
		scanline	= SCANLINE_PRE; 
		ppuCycle	= 0;

		// Reset PPU Registers
		ctrl	= 0;
		mask	= 0;
		status	= 0;

		// Reset RAM
		memset(ciRAM, 0xFF, sizeof(ciRAM));
		memset(oamMem, 0x00, sizeof(oamMem));

		// Reset Screen Buffer
		memset(pixelBuffer,	COLOR_BACKDROP_HEX, sizeof(pixelBuffer));
		Viewer::Reset(); // Debug Screen Buffers

	} // Reset()

} // PPU