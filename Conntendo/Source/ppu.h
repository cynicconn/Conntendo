#pragma once
//----------------------------------------------------------------//
// Emulated NES Picture Processing Unit
//----------------------------------------------------------------//

// Conntendo
#include "common.h"

#define SPRITE_TOTAL 64
#define SPRITE_LIMIT 16 // Default is 8

// PPU Memory Maps
#define MEMMAP_PT				0x0000
#define MEMMAP_PT_END			0x1FFF
#define MEMMAP_NT				0x2000
#define MEMMAP_NT_END			0x3EFF
#define MEMMAP_AT				0x23C0
#define MEMMAP_PALETTE			0x3F00
#define MEMMAP_PALETTE_END		0x3FFF

namespace PPU
{
	enum Scanline
	{
		Invalid,
		VISIBLE,
		POST,
		NMI,
		PRE

	}; // Scanline

	enum Mirroring
	{
		ONESCREEN_LOW,
		ONESCREEN_UP,
		VERTICAL,
		HORIZONTAL,
		FOURSCREEN // uses extraRAM

	}; // Mirroring 

	// Read-Write Functions
	u8 WriteMemory(u16 address, u8 val);
	u8 ReadMemory(u16 address);
	u8 DebugReadMemory(u16 address);

	// NameTable Functions
	u16 GetNameTable(u16 address);
	void SetMirrorMode(Mirroring mode);

	// Run Functions
	void Execute();
	void Reset();
	void ClearOAM();

	// Scanline Functions
	void VisibleScanline(Scanline scan);

	// Debug Display Functions
	bool ToggleHighlightSprites();
	bool ToggleFillSprites();
	bool ToggleDisableBackground();
	bool ToggleDisableSpriteOffset();
	void ToggleNametableViewer(bool toView);
	void TogglePatternTableViewer(bool toView); 

	// Debug Functions
	int GetDebugValue(); 
	void IncrementDebugValue(bool incr);

	// Debug PPU Values
	int GetCycle();
	int GetScanline();
	bool IsRendering(); // for Mappers

	// Special Case
	void DisableCIRAM(bool toDisable);

	enum PPU_MEMMAP
	{
		None,
		CHR,
		Nametable,
		Palette

	}; // PPU_MEMMAP

	enum PPU_CTRL
	{
		NMI_ENABLED			= 0x80,
		MASTER_SLAVE		= 0x40,
		SPR_HEIGHT			= 0x20,
		BKD_TILE_SLCT		= 0x10,
		SPR_TILE_SLCT		= 0x08,
		INCREMENT_MODE		= 0x04,
		NAMETABLE_SLCT_A	= 0x02,
		NAMETABLE_SLCT_B	= 0x01

	}; // PPU_CTRL

	enum PPU_MASK
	{
		//EMPHASIS_RGB = 0xE0,
		EMPHASIS_BLUE			= 0x80,
		EMPHASIS_GREEN			= 0x40,
		EMPHASIS_RED			= 0x20,
		SPR_ENABLE				= 0x10,
		BKD_ENABLE				= 0x08,
		SPR_LEFTCOL_ENABLE		= 0x04,
		BKD_LEFTCOL_ENABLE		= 0x02,
		GREYSCALE				= 0x01

	}; // PPU_MASK

	enum PPU_STATUS
	{
		VBLANK			= 0x80,
		SPR_ZERO_HIT	= 0x40,
		SPR_OVER_EIGHT	= 0x20

	}; // PPU_STATUS

	// VRAM Address (Union is easiest way to manipulate data)
	union PPU_ADDRESS
	{
		struct
		{
			unsigned coarseX	: 5;  // Coarse X
			unsigned coarseY	: 5;  // Coarse Y
			unsigned nameTable	: 2;  // Nametable
			unsigned fineY		: 3;  // Fine Y
		};
		struct
		{
			unsigned low  : 8;
			unsigned high : 7;
		};
		unsigned address : 14;
		unsigned reg	 : 15;
	};

	enum class SPR_ATTR
	{
		COLOR_HI	= 0x03,
		RESERVED	= 0x1C,
		PRIORITY	= 0x20,
		BEHIND_BG	= PRIORITY,
		FLIP_H		= 0x40,
		FLIP_V		= 0x80

	}; // SPR_ATTR

	// Holds all data NES Sprite needs
	struct Sprite
	{
		u8 posY;	// Top Y
		u8 posX;	// Left X

		u8 tileID;	// Tile Index
		u8 index;	// Index in OAM

		u8 attr;	// Attributes

		u8 dataL;	// Low Tile data
		u8 dataH;	// High Tile data

	};// Sprite 

	// For storing "snapshot" of PPU Memory (for savestates)
	struct SaveData
	{
		Mirroring mirrorMode;
		u8 ciRAM[K_2];
		u8 cgRAM[32];
		u8 oamMem[256];

		Sprite oam[SPRITE_LIMIT];
		Sprite secOAM[SPRITE_LIMIT];

		u8 ctrl;
		u8 mask;
		u8 status;

		int scanline;
		int cycle;
		bool isEvenFrame;

		// VRAM Addresses
		PPU_ADDRESS vRamAddr;
		PPU_ADDRESS tempAddr;

		u8 fineX;
		u8 oamAddress;

		// Background Latches
		u8 nameTable;
		u8 attrTable;
		u8 bgLow;
		u8 bgHigh;

		// Background Shift Registers
		u8 attrShiftLow;
		u8 attrShiftHigh;
		u16 bgShiftLow;
		u16 bgShiftHigh;
		bool attrLatchLow;
		bool attrLatchHigh;

		// PPU Temp Storage
		u16	renderAddress;
		u8	memRes;
		u8	memBuffer;
		bool memLatch;

		SaveData() {}; // Default Constructor

		SaveData( Mirroring nMirror, u8 nCI[], u8 nCG[], u8 nOAMMem[], Sprite nOAM[], Sprite nSecOAM[] )
		{
			mirrorMode = nMirror;
			for (int i = 0; i < K_2; i++)
			{
				ciRAM[i] = nCI[i];
			} // for
			for (int i = 0; i < 32; i++)
			{
				cgRAM[i] = nCG[i];
			} // for
			for (int i = 0; i < 256; i++)
			{
				oamMem[i] = nOAMMem[i];
			} // for
			for (int i = 0; i < SPRITE_LIMIT; i++)
			{
				oam[i] = nOAM[i];
			} // for
			for (int i = 0; i < SPRITE_LIMIT; i++)
			{
				secOAM[i] = nSecOAM[i];
			} // for	

		} // SaveData()

	}; // SaveData

	SaveData GrabSaveData();
	void LoadSaveData( SaveData loadedData);

} // PPU