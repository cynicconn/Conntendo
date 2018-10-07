#pragma once
#include "common.h"

namespace Palette
{

#define LOW_CHROMA  150
#define MID_CHROMA  300
#define HIGH_CHROMA 500

// Indices from Default Palette
#define PALETTE_MAGENTA		0x24 
#define PALETTE_RED			0x16 
#define PALETTE_BLACK		0x3F

	enum Colorspace
	{
		NES_COMP,
		NES_RGB,  
		GAMEBOY,
		VIRTUALBOY,
		COMMODORE64,
		CGA,
		EGA
	};

	enum ColorBias
	{
		Black,
		White,
		Red, 
		Cyan,
		Purple,
		Green, 
		Blue,
		Yellow
	};

	void SetPalette(Colorspace palette);
	u32 GetColor(int index);
	void GeneratePalette();

}