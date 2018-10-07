#include "palette.h"

namespace Palette
{
	const float BIAS = 0.66f;

	//////////////// Color Palette Arrays ////////////////
	extern u32 palette_GameBoy[] =
	{
		0xE0F8D0, 0x88C070, 0x306850, 0x081820
	};
	u32 palette_GameBoy_64[64];

	extern u32 palette_VirtualBoy[] =
	{
		0xFF0000, 0xA10000, 0x400000, 0x000000
	};
	u32 palette_VirtualBoy_64[64];

	extern u32 palette_CGA[] =
	{
		0XFFFFFF, 0X00FFFF, 0XFF00FF, 0X000000
	};
	u32 palette_CGA_64[64];

	extern u32 palette_EGA[] =
	{
		0X000000, 0X0000AA, 0X00AA00, 0X00AAAA,
		0XAA0000, 0XAA00AA, 0XAA5500, 0XAAAAAA,
		0X555555, 0X5555FF, 0X55FF55, 0X55FFFF,
		0XFF5555, 0XFF55FF, 0XFFFF55, 0XFFFFFF
	};
	u32 palette_EGA_64[64];

	extern u32 palette_Commodore64[] =
	{
		0X000000, 0XFFFFFF, 0X883932, 0X67B6BD,
		0X8B3F96, 0X55A049, 0X40318D, 0XBFCE72,
		0X8B5429, 0X574200, 0XB86962, 0X505050,
		0X787878, 0X94E089, 0X7869C4, 0X9F9F9F
	};
	u32 palette_Commodore64_64[64];

	extern u32 palette_NES_RGB[] =
	{
		0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400, 0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
		0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10, 0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
		0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044, 0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
		0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8, 0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
	};

	extern u32 palette_NES_Comp[] =
	{
		0X656565, 0X00127D, 0X18008E, 0X360082, 0X56005D, 0X5A0018, 0X4F0500, 0X381900, 0X1D3100, 0X003D00, 0X004100, 0X003B17, 0X002E55, 0X000000, 0X000000, 0X000000,
		0XAFAFAF, 0X194EC8, 0X472FE3, 0X6B1FD7, 0X931BAE, 0X9E1A5E, 0X993200, 0X7B4B00, 0X5B6700, 0X267A00, 0X008200, 0X007A3E, 0X006E8A, 0X000000, 0X000000, 0X000000,
		0XFFFFFF, 0X64A9FF, 0X8E89FF, 0XB676FF, 0XE06FFF, 0XEF6CC4, 0XF0806A, 0XD8982C, 0XB9B40A, 0X83CB0C, 0X5BD63F, 0X4AD17E, 0X4DC7CB, 0X4C4C4C, 0X000000, 0X000000,
		0XFFFFFF, 0XC7E5FF, 0XD9D9FF, 0XE9D1FF, 0XF9CEFF, 0XFFCCF1, 0XFFD4CB, 0XF8DFB1, 0XEDEAA4, 0XD6F4A4, 0XC5F8B8, 0XBEF6D3, 0XBFF1F1, 0XB9B9B9, 0X000000, 0X000000
	};

	// Default Palette
	Colorspace palette = Colorspace::NES_COMP;

	void SetPalette( Colorspace newCS )
	{
		palette = newCS;

	} // SetPalette()

	// Modify color value to increase contrast between simular colors in palette
	inline u32 BiasColor(u32 tryColor)
	{
		int red = (tryColor >> 16) & 0xFF;
		int green = (tryColor >> 8) & 0xFF;
		int blue = (tryColor) & 0xFF;
		int threshold = 10;
		
		if (red > green)
		{
			if (red > blue) // RED
			{
				if ( (red + threshold) > (green + blue) )
				{
					tryColor &= 0xFFCCCC;
				}
			}
		}
		else if (green > blue) // GREEN
		{
			if ((green + threshold) > (red + blue))
			{
				tryColor &= 0xCCFFCC;
			}
		}
		else // BLUE
		{
			if ((blue + threshold) > (green + red))
			{
				tryColor &= 0xCCCCFF;
			}
		}	
		return tryColor & 0xF0F0F0; // remove low-end numbers before returning

	} // BiasColor()

	// Detect a value from the smaller palette to replace the original NES palette
	inline u32 MultiColor(int index, int length, bool isCommodore )
	{
		u32 tryColor	= palette_NES_RGB[index];
		tryColor		= BiasColor(tryColor);
		int tRed		= (tryColor >> 16) & 0xFF;
		int tGreen		= (tryColor >> 8) & 0xFF;
		int tBlue		= (tryColor) & 0xFF;

		int iter = 0;
		int threshold = 10;

		while ( iter < length)
		{
			u32 comColor = (isCommodore) ? palette_Commodore64[iter] : palette_EGA[iter];
			int cRed = (comColor >> 16) & 0xFF;
			int cGreen = (comColor >> 8) & 0xFF;
			int cBlue = (comColor) & 0xFF;

			if (abs(tRed - cRed) <= threshold)
			{
				if (abs(tGreen - cGreen) <= threshold)
				{
					if (abs(tBlue - cBlue) <= threshold)
					{
						return iter;
					}
				}
			}
			iter++;
			if (iter >= length)
			{
				threshold += 10;
				iter = 0;
			}
		} // while

	} // MultiColor()

	// Choose from one of four values to replace the original NES palette
	inline u32 FourColor(int index)
	{
		u32 color	= palette_NES_RGB[index];
		color		&= 0xF0F0F0;
		u32 chroma	= ((color & 0xFF)*BIAS) + ((color >> 8) & 0xFF) + (((color >> 16) & 0xFF)*BIAS);

		if (chroma <= LOW_CHROMA)
		{
			return 3;
		}
		else if (chroma <= MID_CHROMA)
		{
			return 2;
		}
		else if (chroma <= HIGH_CHROMA)
		{
			return 1;
		}
		else
		{
			return 0;
		}

	} // FourColor()

	// Cache 64 Color Palette from smaller palette
	void GeneratePalette()
	{
		for (int i = 0; i < 64; i++)
		{
			palette_GameBoy_64[i]		= palette_GameBoy[FourColor(i)];
			palette_VirtualBoy_64[i]	= palette_VirtualBoy[FourColor(i)];
			palette_Commodore64_64[i]	= palette_Commodore64[MultiColor(i,16,true)];
			palette_CGA_64[i]			= palette_CGA[FourColor(i)];
			palette_EGA_64[i]			= palette_EGA[MultiColor(i, 16,false)];
		} // for

	} // GeneratePalette()

	// Return color from currently selected palette
	u32 GetColor(int index)
	{
		// Out of Range...is this the blacker than black stuff?? Punch Out and Bubble Bobble had issues
		if (index < 0x0 || index > 0x3F)
		{
			index = PALETTE_BLACK; // Last Tile
		}
		switch (palette)
		{
		case Colorspace::NES_COMP:
			return palette_NES_Comp[index];
		case Colorspace::GAMEBOY:
			return palette_GameBoy_64[index];
		case Colorspace::VIRTUALBOY:
			return palette_VirtualBoy_64[index];
		case Colorspace::COMMODORE64:
			return palette_Commodore64_64[index];
		case Colorspace::CGA:
			return palette_CGA_64[index];
		case Colorspace::EGA:
			return palette_EGA_64[index];
		default:
			return palette_NES_RGB[index];
		} // switch

	} // GetColor()

} // Palette