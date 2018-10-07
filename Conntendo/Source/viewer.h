#pragma once
//----------------------------------------------------------------//
// For drawing Debug Views such as NameTable and PatternTable Viewer
//----------------------------------------------------------------//

#include "common.h"

namespace Viewer
{
	// Debug Viewers
	void DrawNametableViewer();
	void DrawPatternTableViewer();
	void PreviewColorPalettes();
	void DrawNametable(int xOff, int yOff, u16 nameTable , u16 attrTable);
	void DrawPatternTable(int xOff, int yOff, bool isLeft);

	// Debug Viewer Helpers
	u8 GrabColorFromPalette(bool forSprite, u8 paletteIndex, u8 colorIndex);
	void DebugDrawSolidTile(u8 row, u8 column, u32 color, u8 size);
	u8 GrabAttribute(u16 attrTable, int tileX, int tileY);

	// Setup
	void Reset();

} // Viewer