#pragma once
//----------------------------------------------------------------//
// The NES CPU, Ricoh 2A03 based on MOS 6502 
//----------------------------------------------------------------//

// Conntendo
#include "common.h"
#include "ppu.h"
#include "mapper.h"

// CPU Memory Maps
#define MEMMAP_RAM				0x0000
#define MEMMAP_RAM_END			0x1FFF
#define MEMMAP_PPU				0x2000
#define MEMMAP_PPU_END			0x3FFF
#define MEMMAP_APU_IO			0x4000
#define MEMMAP_APU_IO_END		0x401F
#define MEMMAP_CARTRIDGE		0x4020
#define MEMMAP_CARTRIDGE_END	0xFFFF

namespace CPU
{
	// Read-Write Functions
	u8 WriteMemory(u16 address, u8 val);
	u8 ReadMemory(u16 address);
	u8 WriteAPU_IO(u16 address, u8 val);
	u8 ReadAPU_IO(u16 address);

	// Interrupt Vectors
	const u16 interuptVector[] = 
	{ 
		0xFFFA, // NMI
		0xFFFC, // Reset
		0xFFFE, // IRQ
		0xFFFE  // Break
	};

	enum CPU_MEMMAP
	{
		None,
		RAM,
		PPU,
		APU_IO,
		Cartridge

	}; // CPU_MEMMAP

	enum AddressMode
	{
		Immediate,
		ZeroPage,
		ZeroPageX,
		Absolute,
		AbsoluteX,
		AbsoluteY,
		IndirectX,
		IndirectY

	}; // AddressMode

	enum InteruptType 
	{ 
		NMI, 
		RESET, 
		IRQ, 
		BRK 

	}; // InteruptType

	// Interupt
	void Interupt(InteruptType interupt);

	// Clear|Set Vectors
	void Set_NMI();
	void Clear_NMI();
	void Set_IRQ();
	void Clear_IRQ();

	enum Flags
	{
		_CARRY				= 0x00,		// 0th Bit
		_ZERO				= 0x01,		// 1st Bit
		_INTERUPT_DISABLE	= 0x02,		// 2nd Bit
		_DECIMAL_MODE		= 0x03,		// 3rd Bit, // NOT USED
		// 5th always set to 1, 4th only set on BRK Interupt
		_OVER_FLOW			= 0x04,		// 6th Bit
		_NEGATIVE			= 0x05,		// 7th Bit

	}; // ProcessorFlags

	// Processor Flags
	struct ProcessorFlag
	{
		bool flag[6];

	public:
		bool& operator[] (const int i) { return flag[i]; }

		void set(u8 pf) 
		{
			flag[_CARRY]			= NTH_BIT(pf, 0); 
			flag[_ZERO]				= NTH_BIT(pf, 1);
			flag[_INTERUPT_DISABLE]	= NTH_BIT(pf, 2);
			flag[_DECIMAL_MODE]		= NTH_BIT(pf, 3); 
			flag[_OVER_FLOW]		= NTH_BIT(pf, 6); 
			flag[_NEGATIVE]			= NTH_BIT(pf, 7);

		} // set()

		u8 get()
		{
			return	flag[_CARRY]			<< 0 |
					flag[_ZERO]				<< 1 |
					flag[_INTERUPT_DISABLE] << 2 |
					flag[_DECIMAL_MODE]		<< 3 |
										  0 << 4 |
										  1 << 5 |
					flag[_OVER_FLOW]		<< 6 |
					flag[_NEGATIVE]			<< 7;

		} // get()

	}; // ProcessorFlag

	// Run Functions
	void PowerOn();
	void RunFrame();
	void Tick();

	// Time Functions
	int GetCycle();
	void AdjustSpeed(double newSpeed);

	// Struct for holding "snapshot" of NES CPU memory to restore later
	struct SaveData
	{
		// CPU Data
		u8 ram[K_2];
		u8 A;
		u8 X;
		u8 Y;
		u8 SP;
		u16 PC;
		ProcessorFlag PF;

		int		cpuCycle;
		int		waitCycles;
		double	timingCycle;

		bool nmiFlag;
		bool irqFlag;
		bool nmiCycled;
		bool irqCycled;
					
		MAPPER::SaveData mapperData;
		PPU::SaveData ppuData;	

		SaveData() {} // Default Constructor 

		SaveData( u8 nRam[], u8 nA, u8 nX, u8 nY, u8 nSP, u16 nPC, ProcessorFlag nPF ) 
		{
			memcpy(ram, nRam, sizeof(ram) );
			A = nA;
			X = nX;
			Y = nY;
			SP = nSP;
			PC = nPC;
			PF = nPF;
		}

		// Save Additional CPU Data ( so constructor parameter list isnt too bloated )
		void SetCPUData(int nCycle, int nWaitCycles, double nTimingCycle, bool nNmiFlag, bool nIrqFlag, bool nNmiCycled, bool nIrqCycled)
		{
			cpuCycle = nCycle;
			waitCycles = nWaitCycles;
			timingCycle = nTimingCycle;
			nmiFlag = nNmiFlag;
			irqFlag = nIrqFlag;
			nmiCycled = nNmiCycled;
			irqCycled = nIrqCycled;

		} // SetCPUData()

		void SetPPUData(PPU::SaveData grabbedData )
		{
			ppuData = grabbedData;

		} // SetPPUData()

		void SetMapperData(MAPPER::SaveData grabbedData)
		{
			mapperData = grabbedData;

		} // SetMapperData()

	}; // SaveData

	// Load/Save Functions
	CPU::SaveData GrabSaveData();
	void LoadSaveData(CPU::SaveData saveData);

} // CPU

