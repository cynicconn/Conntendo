#pragma once
//----------------------------------------------------------------//
// Macros, includes, consts, etc that all classes and files need
//----------------------------------------------------------------//

// STL
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include <string>

// Dev
#include <fstream>
#include <iomanip>
#include <direct.h>

using namespace std;

// STL Colors
#define COLOR_IDLE			Color::White
#define COLOR_ACTIVE		Color::LightSkyBlue
#define COLOR_TOOLBAR		Color::CornflowerBlue
#define COLOR_BACKDROP		Color::FromArgb(0xFF, 0x0, 0x48, 0x48)

// Hex Colors
#define COLOR_BACKDROP_HEX	0x004848 // Conntendo Backdrop Color
#define COLOR_DEBUG_HEX		0xFF00FF // for highlighting Sprite

// Conntendo Extension
#define SAVE_EXT ".connsav"
#define CONFIG_EXT ".conncfg"
#define TEST_EXT ".conntest"

// Extensions
#define DUMP_EXT ".txt"
#define IMAGE_EXT ".connpic" // "png" but custom extension
#define NES_EXT ".nes"

// Dev Flags
#define DEV_BUILD 0
#define USE_COMPRESSION 1
#define DEBUG_DUMP_OPCODES 0

// KB Values
const int K_1		= 1024;		// 0x0400
const int K_2		= 2048;		// 0x0800
const int K_4		= 4096;		// 0x1000
const int K_8		= 8192;		// 0x2000
const int K_16		= 16384;	// 0x4000
const int K_24		= 24576;	// 0x6000
const int K_32		= 32768;	// 0x8000
const int K_64		= 65536;	// 0xFFFF

const int K_128		= 131072;	// 0x00020000
const int K_256		= 262144;	// 0x00040000
const int K_512		= 524288;	// 0x00080000

// Num Types
#define HEX_NUM 16
#define DEC_NUM	10

// Bitwise Macros
#define NTH_BIT(x, n) ( ( (x) >> (n) ) & 1 )
#define IS_SET( reg, mask ) ( reg & (u8)mask )
#define SET_BIT(reg, mask) ( reg |= (u8)mask )
#define CLEAR_BIT(reg, mask) ( reg &= ~(u8)mask )
#define MASK_OFF( reg, mask ) ( reg & mask )

// Helper Macros
#define CLAMP(x,low,high) fmin( high, fmax(low,x) )

// Integer Types
typedef unsigned __int8		u8;
typedef unsigned __int16	u16;
typedef unsigned __int32	u32;
typedef __int8				s8;
typedef __int16				s16;
