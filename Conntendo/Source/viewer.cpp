#include "viewer.h"

// Conntendo
#include "ppu.h"
#include "palette.h"
#include "emulator.h"

namespace Viewer
{
	// Debug Viewer Buffers
	u32 nameTableBuffer[WIDTH_x2 * HEIGHT_x2];		// Screen Buffer 512x480
	u32 patternTableBuffer[WIDTH * WIDTH];			// Screen Buffer 256x256

	// Draw Tile from NameTable
	void DebugDrawTile(int x, int y, u16 ntData, u16 attrData)
	{
		u16 bgShiftL = 0;
		u16 bgShiftH = 0;

		u8 bgL = 0;
		u8 bgH = 0;
		u8 bgAddr = 0;

		for (int i = 0; i < 8; i++) // VERT
		{
			int iter = (i == 0) ? 0 : (i+1); // neccessary to grab first row

			// Fetch low and high order byte of an 8x1 pixel sliver
			bool bgTableSelected	= (PPU::DebugReadMemory(0x2000) & 0x10);
			u16 bgTableAddress		= bgTableSelected ? K_4 : 0;
			u16 bgAddr				= bgTableAddress + (ntData * 16) + iter;
			bgL						= PPU::DebugReadMemory(bgAddr);
			bgH						= PPU::DebugReadMemory(bgAddr+8);

			// Fill bg Low and High with first two lines
			if (i == 0)
			{
				bgShiftL	= bgL << 8;
				bgShiftH	= bgH << 8;
				bgAddr		= bgTableAddress + (ntData * 16) + 1;
				bgL			= PPU::DebugReadMemory(bgAddr);
				bgH			= PPU::DebugReadMemory(bgAddr + 8);
			}

			bgShiftL = (bgShiftL & 0xFF00) | bgL;
			bgShiftH = (bgShiftH & 0xFF00) | bgH;

			for (int j = 0; j < 8; j++) // HOR
			{
				u8 palette = 0;
				int yOffset = (i + (y * 8));
				int tileOffset = (WIDTH_x2 * yOffset);
				int locOffset = (x * 8) + j;

				palette = (NTH_BIT(bgShiftH, 15) << 1) | NTH_BIT(bgShiftL, 15); // Get TileMap Data
				if (palette)
				{
					palette |= attrData << 2; // 4 Bit Color
				}
				bgShiftL <<= 1;
				bgShiftH <<= 1;

				// Color Pixel
				u8 colorIndex = PPU::DebugReadMemory(MEMMAP_PALETTE + palette);
				nameTableBuffer[tileOffset + locOffset] = Palette::GetColor(colorIndex);
			} // for

		} // for

	} // DebugDrawTile()

	// Grab from Attribute Table
	u8 GrabAttribute( u16 attrTable, int tileX, int tileY )
	{
		int attrAddr = attrTable + ((tileY >> 2) << 3) + (tileX >> 2);
		u8 attrVal = PPU::DebugReadMemory(attrAddr);

		// Shift Attribute based on Quadrant
		int corner = ((tileY & 2) << 1) + (tileX & 2);
		attrVal &= (3 << corner);
		attrVal >>= corner;

		return attrVal;

	} // GrabAttribute()

	void DrawNametable(int xOff, int yOff, u16 nameTable, u16 attrTable)
	{
		int nIter = 0;

		for (int v = 0; v < 30; v++) // Vert
		{
			for (int h = 0; h < 32; h++) // Hor
			{
				u16 tileAddr	= nameTable | (nIter & 0x0FFF);
				u8 ntData		= PPU::DebugReadMemory(tileAddr);
				u8 attrData		= GrabAttribute(attrTable, h, v);

				DebugDrawTile((xOff + h), (yOff + v), ntData, attrData);
				nIter++;
			} // for
		} // for

	} // DrawNametable()

	// Draw Tile from Pattern Table
	void DebugDrawTilePT(int offset, u8 lowTileData, u8 highTileData)
	{
		for (int i = 0; i < 8; i++)
		{
			u8 colorIndex = (lowTileData & 0x01) + ((highTileData & 0x01) << 1);
			lowTileData >>= 1;
			highTileData >>= 1;
			u8 index = GrabColorFromPalette(false, 0, colorIndex);
			patternTableBuffer[offset + (8 - i)] = Palette::GetColor(index);
		} // for

	} // DebugDrawTilePT()

	void DrawPatternTable(int xOff, int yOff, bool isLeft)
	{
		u16 patternTab = isLeft ? 0 : K_4;
		u16 tableOffset = isLeft ? 0 : 128;
		for (int h = 0; h < 16; h++) // VERT
		{
			for (int i = 0; i < 16; i++) // HOR
			{
				for (int j = 0; j < 8; j++) // Tile Row
				{
					u16 lowPlaneAddr = patternTab + j + (i * 16) + (WIDTH * h);
					u16 highPlaneAddr = lowPlaneAddr + 8;
					u8 lowTileData = PPU:: DebugReadMemory(lowPlaneAddr);
					u8 highTileData = PPU:: DebugReadMemory(highPlaneAddr);
					int offset = tableOffset + ((WIDTH * 8) * h) + (WIDTH * j) + (i * 8);
					DebugDrawTilePT(offset, lowTileData, highTileData);
				}
			} // for
		} // for

	} // DrawPatternTable()

	// Draw an nxn Solid Colored Tile
	void DebugDrawSolidTile(u8 row, u8 column, u32 color, u8 size)
	{
		int offset = (row*K_2) + (column*size);
		for (int i = 0; i < size; i++)
		{
			for (int j = 0; j < size; j++)
			{
				int row = i * 256;
				int pos = offset + row + j;
				patternTableBuffer[pos] = color;
			}
		} // for

	} // DebugDrawSolidTile()

	  // grab one of four color indices from background palette
	u8 GrabColorFromPalette(bool forSprite, u8 paletteIndex, u8 colorIndex)
	{
		// check if out of range 
		if (paletteIndex < 0 || paletteIndex > 3 || colorIndex < 0 || colorIndex > 3)
		{
			return PALETTE_BLACK; // Last Color
		}
		paletteIndex += (forSprite) ? 0x0C : 0; // offset to Sprite Palettes
		u16 paletteAddress = MEMMAP_PALETTE + paletteIndex + colorIndex;
		return PPU:: DebugReadMemory(paletteAddress);

	} // GrabColorFromPalette()

	// Draw the Color Palettes (4 Colors each)
	void PreviewColorPalettes()
	{
		u16 address = MEMMAP_PALETTE;
		int column	= 1;
		int row		= 17;
		u8 tileSize = 12;
		u32  universalBGColor = Palette::GetColor(PPU:: DebugReadMemory(MEMMAP_PALETTE));

		for (int pal = 0; pal < 8; pal++) // 4 Background Palettes, 4 Sprite Palettes
		{
			for (int col = 0; col < 4; col++) // 3 unique per palette
			{
				u8 palette = PPU:: DebugReadMemory(address + col);
				u32 color = (col == 0) ? universalBGColor : Palette::GetColor(palette);
				DebugDrawSolidTile(row, column, color, tileSize);
				column++;
			} // for

			column++;
			address += 0x04;
			if (pal == 3) // New row for Sprite Palettes
			{
				column = 1;
				row += 2;
			}

		} // for

	} // PreviewColorPalettes()

	// Draw the two Pattern Tables and Color Palettes
	void DrawPatternTableViewer()
	{
		DrawPatternTable(0, 0, true); // Pattern Table 0
		DrawPatternTable(0, 0, false); // Pattern Table 1
		PreviewColorPalettes();

		Emulator::NewDebugFrame(patternTableBuffer, false);

	} // DrawPatternTableViewer()

	// Render the four Nametables
	void DrawNametableViewer()
	{
		DrawNametable(0,   0, 0x2000, 0x23C0); // Top Left
		DrawNametable(32,  0, 0x2400, 0x27C0); // Top Right
		DrawNametable(0,  30, 0x2800, 0x2BC0); // Bottom Left
		DrawNametable(32, 30, 0x2C00, 0x2FC0); // Bottom Right

		// Draw Black Borders
		int padding = 1;
		for (int i = 0; i < WIDTH_x2; i++)
		{
			for (int j = -padding; j < padding; j++)
			{
				nameTableBuffer[(WIDTH_x2 * (HEIGHT + j)) + i] = Palette::GetColor(PALETTE_BLACK);
			} // for
		} // for
		for (int i = 0; i < HEIGHT_x2; i++)
		{
			for (int j = -padding; j < padding; j++)
			{
				nameTableBuffer[(WIDTH_x2 * i) + (WIDTH + j)] = Palette::GetColor(PALETTE_BLACK);
			} // for
		} // for
		Emulator::NewDebugFrame(nameTableBuffer, true);

	} // DrawNametableViewer()

	void Reset()
	{
		memset(nameTableBuffer, COLOR_BACKDROP_HEX, sizeof(nameTableBuffer));
		memset(patternTableBuffer, COLOR_BACKDROP_HEX, sizeof(patternTableBuffer));

	} // Reset()

} // Viewer
