#include "cpu.h"

// Conntendo
#include "cartridge.h"
#include "apu.h"
#include "ppu.h"
#include "joypad.h"
#include "emulator.h"
#include "dev.h"

// STL
#include <sstream>

// Addresses
#define PPUOAMDATA			0x2014
#define OAMDMA				0x4014
#define APU_STATUS			0x4015
#define JOYPAD0				0x4016
#define JOYPAD1_APU_COUNTER 0x4017

namespace CPU
{
	// Consts
	const double TOTAL_CYCLES = 29781; // CPU Frame Timing

	// CPU Cycles
	int		cpuCycle	= 0;	// Number of times CPU has been called 
	int		waitCycles	= 0;	// the number of CPU frames to stall while PPU Ticks
	double	timingCycle;		// For timing CPU Frame
	int GetCycle() { return cpuCycle; }

	// CPU Tweaks
	double emulatorSpeed = 1.0f;

#if DEBUG_DUMP_OPCODES
	// Debug Printing
	string debugOpCodeResult = ""; // result from opCode(make sure blank firstline
	bool debugPrintOpCode = false; // if true, debugVal will be printed next line
	std::stringstream debugStream;
#endif

	// CPU Blocks
	u8 ram[K_2];		// 2K CPU RAM
	u16 PC;				// Program Counter
	u8 A;				// Accumalator
	u8 X, Y;			// Index Registers
	u8 SP;				// Stack Pointer
	ProcessorFlag PF;

	// Interupt Flags
	bool nmiFlag;
	bool irqFlag;
	bool nmiCycled = false; // dont verify NMI until after next CPU instruction
	bool irqCycled = false; // dont verify IRQ until after next CPU instruction

	// Set/Clear Interupts
	void Set_NMI()		{ nmiFlag = true; nmiCycled = true; }
	void Clear_NMI()	{ nmiFlag = false; }
	void Set_IRQ()		{ irqFlag = true; irqCycled = true; }
	void Clear_IRQ()	{ irqFlag = false; }
	 
	// Return current cpu timing
	inline long elapsed() 
	{ 
		return TOTAL_CYCLES - timingCycle; 

	} // elapsed()

// Fetch Parameter ( based on AddressMode ) Macro
#define FETCH_PARAMS u16 addr = AddrMode(); u8 val = read8(addr)
#define PPU_EXECUTE PPU::Execute(); PPU::Execute(); PPU::Execute()
#define TICK	Tick()
#define TICK_2	Tick(); Tick()
#define TICK_3	Tick(); Tick(); Tick()

//-------------- Debug Print Macros -------------- //

#if DEBUG_DUMP_OPCODES
#define PFHexPrint ( (int)PF[_NEGATIVE] << 7 ) + ( (int)PF[_OVER_FLOW] << 6 ) + ( 1 << 5 ) + ( 0 << 4 ) + \
	( (int)PF[_DECIMAL_MODE] << 3 ) + ( (int)PF[_INTERUPT_DISABLE] << 2 ) + ( (int)PF[_ZERO] << 1 ) + ( (int)PF[_CARRY] << 0 )
#define PFPrint  (bool)PF[_NEGATIVE] << (bool)PF[_OVER_FLOW] << "1" << (bool)PF[_BREAK_COMMAND] << "  " << (bool)PF[_DECIMAL_MODE] << (bool)PF[_INTERUPT_DISABLE] << (bool)PF[_ZERO] << (bool)PF[_CARRY]

#define DebugPrint(opCodeName) string opCodeMessage = (debugPrintOpCode) ? "\t" + debugOpCodeResult + "\n" : debugOpCodeResult; \
	debugStream << opCodeMessage \
	<< "0x" << std::hex << std::uppercase << std::setw(4) << PC - 1 << "\t" << opCodeName << "\t" \
	<< std::setfill('0') << std::uppercase << std::hex \
	<< " A: " << std::setw(2) << (int)A << " X: " << std::setw(2) << (int)X << " Y: " << std::setw(2) << (int)Y \
	<< " PF: " << PFHexPrint << " SP: " << (int)SP << std::dec << " CYC: " << std::setw(3) << PPU::GetCycle()-3 << " SCN: " << std::setw(3) << PPU::GetScanline(); \
	debugPrintOpCode = false; \
	debugOpCodeResult = "\n"

#define DebugTo_Hex(prefix, val) std::stringstream sstream; sstream << prefix << std::hex << std::uppercase << (int)val; string hexVal = sstream.str()
#define DebugTo_Hex2(prefix, val2) std::stringstream sstream2; sstream2 << prefix << std::hex << std::uppercase << (int)val2; string hexVal2 = sstream2.str()
#define DebugOpCodeResult(val) debugOpCodeResult = val; debugPrintOpCode = true

#define DebugOpCode(val) DebugTo_Hex("0x", val); DebugOpCodeResult(hexVal)
#define DebugOpCodeDummy() DebugTo_Hex("0x", dummyRead8(PC-1)); DebugOpCodeResult(hexVal)
#define DebugOpCodePrefix(prefix, val) DebugTo_Hex( prefix, val); DebugOpCodeResult(hexVal)
#define DebugOpCodeStore(address, val) DebugTo_Hex("$", address); DebugTo_Hex2( "0x", val); string output = hexVal + " = " + hexVal2; DebugOpCodeResult(output)
#define DebugOpCodeLoad(address, val) DebugTo_Hex("$", address); DebugTo_Hex2( "0x", val); string output = hexVal2 + " <- " + hexVal; DebugOpCodeResult(output)
#define DebugOpCodeBool(isTrue) if (isTrue) { DebugOpCodeResult("-->"); } else { DebugOpCodeResult("---"); }
#else
#define DebugTo_Hex(prefix, val);
#define DebugTo_Hex2(prefix, val2);
#define DebugOpCodeResult(val);
#define DebugOpCode(val);
#define DebugOpCodeDummy();
#define DebugOpCodePrefix(prefix, val);
#define DebugOpCodeStore(address, val);
#define DebugOpCodeLoad(address, val);
#define DebugOpCodeBool(val);
#endif

//------------------------------------------ //

	//-------------- Emualtor Settings -------------- //
	void AdjustSpeed(double newSpeed)
	{
		emulatorSpeed = newSpeed;
		string speedMessage = "SPEED: x" + to_string(newSpeed);
		speedMessage.erase(speedMessage.find('.') + 3, std::string::npos);
		Emulator::ShowMessage(speedMessage);

	} // AdjustSpeed()

	//-------------- SaveState -------------- //
	CPU::SaveData GrabSaveData()
	{
		CPU::SaveData savedData(ram, A, X, Y, SP, PC, PF);
		savedData.SetCPUData(cpuCycle, waitCycles, timingCycle, nmiFlag, irqFlag, nmiCycled, irqCycled);
		savedData.SetPPUData( PPU::GrabSaveData() );
		savedData.SetMapperData( Cartridge::GetMapper()->GrabSaveData() );
		return savedData;

	} // GrabSaveData()

	void LoadSaveData(CPU::SaveData saveData)
	{
		// Load CPU Data
		A = saveData.A;
		X = saveData.X;
		Y = saveData.Y;
		SP = saveData.SP;
		PC = saveData.PC;
		PF = saveData.PF;

		cpuCycle = saveData.cpuCycle;
		waitCycles = saveData.waitCycles;
		timingCycle = saveData.timingCycle;
		memcpy(ram, saveData.ram, K_2);

		nmiFlag = saveData.nmiFlag;
		irqFlag = saveData.irqFlag;
		nmiCycled = saveData.nmiCycled;
		irqCycled = saveData.irqCycled;

		// Load PPU and Mapper Data
		Cartridge::GetMapper()->LoadSaveData(saveData.mapperData);
		PPU::LoadSaveData(saveData.ppuData);

	} // LoadSaveData()

	// Return location to Read/Write to
	CPU_MEMMAP GetMapLoc(u16 address)
	{
		if (address <= MEMMAP_RAM_END)
		{
			return CPU_MEMMAP::RAM;
		}
		else if (address >= MEMMAP_PPU && address <= MEMMAP_PPU_END)
		{
			return CPU_MEMMAP::PPU;
		}
		else if (address >= MEMMAP_APU_IO && address <= MEMMAP_APU_IO_END)
		{
			return CPU_MEMMAP::APU_IO;
		}
		else if (address >= MEMMAP_CARTRIDGE && address <= MEMMAP_CARTRIDGE_END)
		{
			return CPU_MEMMAP::Cartridge;
		}
		return CPU_MEMMAP::None;

	} // GetMapLoc()

	//-------------- Read/Write Functions --------------//

	// write to 8-bit address
	inline u8 write8(u16 addr, u8 val)
	{
		TICK;
		return WriteMemory(addr, val);

	} // write8()

	// read and return LSB 8-bit value
	inline u8 read8(u16 addr)
	{
		TICK;
		u8 lsb = ReadMemory(addr);
		return lsb;

	} // read8()

	// Dummy Read for debugging
	inline u8 dummyRead8(u16 addr)
	{
		u8 lsb = ReadMemory(addr);
		return lsb;

	} // dummyRead8()

	// read from A and B, then return merged 16-bit value
	inline u16 directRead16(u16 addrA, u16 addrB) 
	{ 
		return (read8(addrA) | (read8(addrB) << 8)); 

	} // directRead16()

	// pass address ( and next address ) and return merged 16-bit value
	inline u16 read16(u16 addr) 
	{ 
		return directRead16( addr, addr+1 );

	} // read16()

	// Push onto Stack and adjust Stack Pointer
	inline u8 Push(u8 val)
	{ 
		u16 pushAddr = 0x0100 + SP;
		u8 writeVal = write8(pushAddr, val);
		SP--;
		return writeVal;

	} // Push()

	// Pop from Stack and adjust Stack Pointer
	inline u8 Pop() 
	{ 
		SP++;
		return read8(0x0100 + SP);

	} // Pop()

	// Upload 256 bytes of data from CPU page $XX00-$XXFF to the internal PPU OAM (CPU is suspended while the transfer is taking place)
	inline void DMA_OAM(u16 bank)
	{
		// not counting OAMDMA write tick, this takes 513 CPU Cycles
		TICK; 
		if (cpuCycle & 0x01)
		{
			TICK; // +1 on Odd CPU Cycles (514)
		}
		// PPU OAMDATA Write will Increment each times its written to
		for (int i = 0; i < 256; i++)
		{
			u8 val = read8(bank + i);
			write8(PPUOAMDATA, val); 
		} // for

	} // DMA_OAM()

	u8 WriteAPU_IO(u16 address, u8 val)
	{
		if (address == OAMDMA)
		{
			u16 bank = val * 0x0100;
			CPU::DMA_OAM(bank);
		}
		else if (address == JOYPAD0)
		{
			Joypad::ToggleStrobe(val & 0x01);
		}
		else if (address == JOYPAD1_APU_COUNTER)
		{
			return APU::write8(elapsed(), address, val);
		}
		else // APU
		{
			return APU::write8(elapsed(), address, val);
		}

	} // WriteAPU_IO()

	u8 ReadAPU_IO(u16 address)
	{
		if (address == JOYPAD0)
		{
			return Joypad::GetInput(0);
		}
		else if (address == JOYPAD1_APU_COUNTER)
		{
			return Joypad::GetInput(1);
		}
		else if (address == APU_STATUS)
		{
			return APU::read8( elapsed() ); 
		}
		else
		{
			return 0;
		}

	} // ReadAPU_IO()

	u8 WriteMemory(u16 address, u8 val)
	{
		u8* writeVal;

		switch (GetMapLoc(address))
		{
		case CPU_MEMMAP::RAM:
			return ram[address & 0x07FF] = val;
		case CPU_MEMMAP::PPU:
			return PPU::WriteMemory(address, val); 
		case CPU_MEMMAP::APU_IO:
			return WriteAPU_IO(address, val);
		case CPU_MEMMAP::Cartridge:
			return Cartridge::WritePRG(address, val);
		} // switch

		return val;

	} // WriteMemory()

	u8 ReadMemory(u16 address)
	{
		u8* readVal;

		switch (GetMapLoc(address))
		{
		case CPU_MEMMAP::RAM:
			return ram[address & 0x07FF];
		case CPU_MEMMAP::PPU:
			return PPU::ReadMemory(address); 
		case CPU_MEMMAP::APU_IO:
			return ReadAPU_IO(address);
		case CPU_MEMMAP::Cartridge:
			return Cartridge::ReadPRG(address);
		} // switch

		return 0;

	} // ReadMemory()

	// Decrement the Cycle Clock
	void Tick() 
	{ 
		waitCycles++; // Increment number of cycles to wait for PPU
		timingCycle -= (1.0f / emulatorSpeed);

	} // Tick()

	//-------------- Flag Updating --------------//

	// Update two Flags; Negative and Zero
	inline void Update_NZ(u8 val)
	{ 
		PF[_NEGATIVE]	= val & 0x80; 
		PF[_ZERO]		= (val == 0);

	} // Update_NZ()

	// return true if page is crossed (0xFF->0x00)
	inline bool Cross(u16 address, u8 i )
	{ 
		return ( (address + i) & 0xFF00) != (address & 0xFF00); 

	} // Cross()

	//-------------- Processor Flag Instructions --------------//

	void ClearFlag(Flags flag)
	{
		PF[flag] = false; 
		if (flag == _INTERUPT_DISABLE)
		{
			irqCycled = true;
		}
		TICK;

	} // ClearFlags

	void SetFlag(Flags flag)
	{ 
		PF[flag] = true; 
		TICK; 

	} // SetFlags

	//-------------- Stack Instructions --------------//
	void TSX()
	{
		TICK; 
		X = SP; 
		Update_NZ(X);
		DebugOpCodeDummy();

	} // Transfer Stack Pointer to X

	void TXS()
	{
		TICK; 
		SP = X;
		DebugOpCodeDummy();

	} // Transfer X to Stack Pointer

	void PLA() 
	{ 
		TICK_2;
		A = Pop(); 
		Update_NZ(A);

	} // Pull Accumuluator

	void PHA()
	{
		TICK; 
		Push(A); 

	} // Push Accumuluator

	void PLP()
	{
		TICK_2;
		PF.set( Pop() );

	} // Pull Processor Status

	void PHP()
	{
		TICK;  
		u8 pushVal = PF.get() | 0x30; // "Bits 4 and 5 are always set when using PHP "the B Flag"
		Push( pushVal );

	} // Push Processor Status

	//-------------- Register Instructions --------------//
	void TAX()
	{
		TICK; 
		X = A; 
		Update_NZ(X); 

	} // Transfer Accumulator to X ( Affects Flags: N, Z )

	void TXA()
	{
		TICK; 
		A = X;
		Update_NZ(A);

	} // Transfer X to Accumulator ( Affects Flags: N, Z )

	void DEC( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		u8 test = write8(addr, --val);
		Update_NZ(test);
		TICK;

	} // Decrement Memory ( Affects Flags: N, Z )

	void INC( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		u8 test = write8(addr, ++val);
		Update_NZ(test);
		TICK;

	} // Increment Memory ( Affects Flags: N, Z )

	void DEX()
	{
		Update_NZ(--X); 
		TICK; 

	} // Decrement X Register ( Affects Flags: N, Z )

	void INX()
	{
		Update_NZ(++X); 
		TICK; 

	} // Increment X Register ( Affects Flags: N, Z )

	void TAY()
	{
		Y = A; 
		Update_NZ(Y);
		TICK;

	} // Transfer Accumulator to Y ( Affects Flags: N, Z )

	void TYA()
	{
		A = Y;
		Update_NZ(A);
		TICK;

	} // Transfer Y to Accumulator ( Affects Flags: N, Z )

	void DEY()
	{
		Update_NZ(--Y);
		TICK;

	} // Decrement Y Register ( Affects Flags: N, Z )

	void INY()
	{
		Update_NZ(++Y); 
		TICK;

	} // Increment X Register ( Affects Flags: N, Z )

	//-------------- Addressing Modes --------------//

	// return opperand's direct value ( 8-bit address )
	inline u16 immediate8()	{ return PC++; }

	// return opperand's direct value ( full 16-bit address )
	inline u16 immediate16(){ PC += 2; return PC-2; }

	// use operand's value as address and return THAT value ( first 256 bytes of Memory Map )
	inline u16 zeropage()	{ return read8( immediate8() ); };

	// return zeropage + X register value
	inline u16 zeropageX()	{ TICK; return (zeropage() + X) % 0x100; }

	// return zeropage + Y register value
	inline u16 zeropageY()	{ TICK; return (zeropage() + Y) % 0x100; }

	// return content from operand's address ( full 16-bit address )
	inline u16 absolute()	{ return read16( immediate16() ); }

	// AbsoluteX Exception: add 1 cycle ( boundary crossed or not )
	inline u16 absoluteX_Exc()	{ TICK; return absolute() + X; }

	// same as Absolute(), but add X value to return value
	inline u16 absoluteX()	{ u16 val = absolute(); if ( Cross(val, X) ) TICK; return val + X; }

	// same as Absolute(), but add Y value to return value
	inline u16 absoluteY()	{ u16 val = absolute(); if ( Cross(val, Y) ) TICK; return val + Y; }

	// use zeropageX() value as address and return THAT value + X value
	inline u16 indirectX()	{ u16 indir = zeropageX(); return directRead16(indir, (indir + 1) % 0x100); }

	// Indirect Exception, Does not Tick
	inline u16 indirectY_Exc()	{ u16 indir = zeropage(); return directRead16(indir, (indir + 1) % 0x100) + Y; }

	// use zeropageY() value as address and return THAT value + Y value
	inline u16 indirectY()	{ u16 indir = indirectY_Exc(); if ( Cross(indir - Y, Y) ) TICK; return indir; }

	//------------------------------------------//

	//-------------- 6502 Instructions --------------// 
	void ADC( u16(*AddrMode)() )
	{ 
		FETCH_PARAMS; 
		s16 res			= A + val + PF[_CARRY]; 
		PF[_CARRY]		= (res & 0x100);
		PF[_OVER_FLOW]	= (A ^ res) & (val ^ res) & 0x80;
		A				= res & 0xFF;
		Update_NZ(A);
		DebugOpCode(res);

	} // Add with Carry ( Affects Flags: N, V, Z, C )

	void SBC( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		s16 res			= A - val - !PF[_CARRY]; 
		PF[_CARRY]		= !(res & 0x100);
		PF[_OVER_FLOW]	= (A ^ res) & (~val ^ res) & 0x80;
		A				= res & 0xFF;
		Update_NZ(A);
		DebugOpCode(res);

	} // Subtract with Carry ( Affects Flags: N, V, Z, C )

	void AND( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		A &= val;
		Update_NZ(A);

	} // Bitwise & with Accumulator ( Affects Flags: N, Z )

	void ORA( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		A |= val;
		Update_NZ(A);
		DebugOpCode(val);

	} // Bitwise | with Accumulator ( Affects Flags: N, Z )

	void EOR( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		A ^= val;
		Update_NZ(A);

	} // Bitwise XOR with Accumulator ( Affects Flags: N, Z )

	void ASL_A()
	{
		PF[_CARRY] = A & 0x80;
		A <<= 1;
		Update_NZ(A);
		TICK;
		DebugOpCode(A);

	} // Arithmetic Shift Left on Accumulator ( Affects Flags: N, Z, C )

	void ASL( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		PF[_CARRY] = (val & 0x80); 
		u8 test = write8(addr, val << 1);
		Update_NZ(test);
		TICK;
		DebugOpCode(val << 1);

	} // Arithmetic Shift Left ( Affects Flags: N, Z, C )

	// Same as AND, but only sets the flags, and throws away the result
	void BIT( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		PF[_ZERO]		= !(A & val); 
		PF[_NEGATIVE]	= val & 0x80; 
		PF[_OVER_FLOW]	= val & 0x40;

	} // Test Bits ( Affects Flags: N, V, Z )

	void CMP( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		Update_NZ(A - val); 
		PF[_CARRY] = (A >= val);

	} // Compare Accumulator ( Affects Flags: N, Z, C )

	void CPX( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		Update_NZ(X - val); 
		PF[_CARRY] = (X >= val);

	} // Compare X Register ( Affects Flags: S, Z, C )

	void CPY( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		Update_NZ(Y - val); 
		PF[_CARRY] = (Y >= val);
		DebugOpCodeDummy();

	} // Compare Y Register ( Affects Flags: S, Z, C )

	void STA( u16 addr ) 
	{
		write8( addr, A );
		DebugOpCodeStore(addr, A); 

	} // Store in Acculumulator ( Affects Flags: -- )

	void STA(AddressMode mode)
	{
		TICK;
		switch (mode)
		{
		case AddressMode::IndirectY:
			STA( indirectY_Exc() );
			break;
		case AddressMode::AbsoluteX:
			STA(absolute() + X);
			break;
		case AddressMode::AbsoluteY:
			STA(absolute() + Y);
			break;
		}

	} // // Store in Acculumulator ( Affects Flags: -- )

	void STX(u16 addr)
	{
		write8(addr, X);
		DebugOpCodeStore(addr, X);

	} // Store X Register ( Affects Flags: -- )

	void STY(u16 addr)
	{
		write8(addr, Y);
		DebugOpCodeStore(addr, Y);

	} // Store Y Register( Affects Flags: -- )

	void LDA( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		A = val;
		Update_NZ(A);
		DebugOpCodeLoad(addr, A);

	} // Load to Acculumulator ( Affects Flags: S, Z )

	void LDX( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		X = val;
		Update_NZ(X);
		DebugOpCodeLoad(addr, X);

	} // Load to X Register ( Affects Flags: S, Z )

	void LDY( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		Y = val;
		Update_NZ(Y);
		DebugOpCodeLoad(addr, Y);

	} // Load to Y Register ( Affects Flags: S, Z )

	void LSR( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		PF[_CARRY] = val & 0x01;
		u8 shiftVal = (val >> 1) & 0x7F;
		write8(addr, shiftVal);
		Update_NZ(shiftVal);
		TICK;
		DebugOpCode(shiftVal);

	} // Logical Shift Right ( Affects Flags: N, Z, C )

	void LSR_A()
	{
		PF[_CARRY] = A & 0x01; // bit 0 shifted into Carry Flag
		A >>= 1; // shift right, ( bit 7 should be zero )
		Update_NZ( A );
		TICK;
		DebugOpCode(A);

	} // Logical Shift Accumulator Right ( Affects Flags: N, Z, C )

	void ROR( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		u8 carry = PF[_CARRY] << 7;
		PF[_CARRY] = val & 0x01; 
		u8 test = write8(addr, carry | (val >> 1));
		Update_NZ(test);
		TICK;
		DebugOpCodeDummy();

	} // Roate Right ( Affects Flags: N, Z, C )

	void ROR_A()
	{
		u8 carry = PF[_CARRY] << 7;
		PF[_CARRY] = A & 0x01; 
		A = (carry | (A >> 1));
		Update_NZ(A); 
		TICK;
		DebugOpCode(A);

	} // Roate Accumulator Right ( Affects Flags: N, Z, C )

	void ROL( u16(*AddrMode)() )
	{
		FETCH_PARAMS; 
		u8 carry = PF[_CARRY]; 
		PF[_CARRY] = val & 0x80; 
		u8 test = write8(addr, (val << 1) | carry);
		Update_NZ(test);
		TICK;
		DebugOpCodeDummy();

	} // Roate Left ( Affects Flags: N, Z, C )

	void ROL_A()
	{
		u8 carry = PF[_CARRY]; 
		PF[_CARRY] = A & 0x80; 
		A = ((A << 1)) | carry;
		Update_NZ(A);
		TICK;
		DebugOpCode(A);

	} // Roate Accumulator Left ( Affects Flags: N, Z, C )

	// Flow Control Instructions
	void Branch(Flags flag, bool val)
	{ 
		s8 jumpTo = read8( immediate8() ); 
		bool shouldBranch = (PF[flag] == val);
		int printValue = PC + jumpTo;

		if (shouldBranch)
		{ 
			TICK; 
			bool checkPlusOne = (PC & 0x0F00) != (printValue & 0x0F00); // +1 cycle if to new page 
			if (checkPlusOne) // +1 cycle if to new page 
			{
				TICK;
			}
			PC += jumpTo;
		}
		DebugOpCodeBool(shouldBranch);

	} // Branch()

	void BREAK()
	{
		Interupt(InteruptType::BRK);

	} // Break

	void JMP() 
	{ 
		PC = read16( immediate16() ); 
		DebugOpCodePrefix( "$", PC );

	} // Jump

	void JMP_INDR()
	{ 
		u16 iAddr  = read16(immediate16());
		u16 iAddrB = (iAddr & 0xFF00) | ((iAddr + 1) & 0x00FF);
		PC = directRead16( iAddr, iAddrB);

	} // Indirect Jump

	void JSR()
	{
		u16 store = PC + 1; 
		Push( (store >> 8) & 0xFF ); // high byte
		Push( store & 0xFF ); // low byte
		PC = read16( immediate16() );
		TICK;
		DebugOpCodePrefix("$", PC);

	} // Jump to Subroutine 

	//-------------- Return Instructions --------------//

	void RTS()
	{
		u8 dummyFetch = read8(PC++);
		PC = (Pop() | (Pop() << 8)) + 1; 
		TICK_2;

	} // Return from Subroutine

	void RTI()
	{ 
		u8 dummyFetch = dummyRead8(PC++);
		PLP();
		PC = Pop() | (Pop() << 8); 

	} // Return from Interupt

	void NOP() 
	{ 
		TICK; 

	} // No Operation ( Legal )

	//-------------- Unofficial Instructions --------------//

	void LAX( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		A = val;
		X = val;
		Update_NZ(val);
		DebugOpCode(val);

	} // Combo of LDA and LDX

	void SAX( u16(*AddrMode)() )
	{
		u16 addr = AddrMode();
		u8 res = A & X;
		write8( addr, res);
		DebugOpCode(res);

	} // Stores the bitwise AND of A and X ( no affected Flags )

	void DCP( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		val--;
		write8(addr, val);
		Update_NZ(A - val);
		PF[_CARRY] = (A >= val);
		TICK;
		DebugOpCode(val);

	} // Combo of Dec and CMP ( Affects Flags: N, Z, C )

	void ISB( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		val++;
		write8(addr, val);
		s16 res = A - val - !PF[_CARRY];
		PF[_CARRY] = !(res & 0x100);
		PF[_OVER_FLOW] = (A ^ res) & (~val ^ res) & 0x80;
		A = res & 0xFF;
		Update_NZ(A);
		TICK;

	} // Equivalent to INC value then SBC value ( Affects Flags: N, V, Z, C )

	void SLO( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		PF[_CARRY] = (val & 0x80);
		write8(addr, val <<= 1);
		A |= val;
		Update_NZ(A);
		TICK;

	} // Equivalent to ASL value then ORA value ( Affects Flags: N, Z, C )

	void RLA( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		u8 carry = PF[_CARRY];
		PF[_CARRY] = val & 0x80;
		u8 res = (val << 1) | carry;
		write8( addr, res );
		A &= res;
		Update_NZ(A);
		TICK;

	} // Combo of ROL plus AND ( Affects Flags: V, N, Z, C )...no V?

	void SRE( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		PF[_CARRY] = val & 0x01;
		u8 res = (val >> 1) & 0x7F;
		write8(addr,res);
		A ^= res;
		Update_NZ(A);
		TICK;

	} // Combo of LSR and EOR ( Affects Flags: N, Z, C )

	void RRA( u16(*AddrMode)() )
	{
		FETCH_PARAMS;
		u8 carry = PF[_CARRY] << 7;
		PF[_CARRY] = val & 0x01;
		u8 newVal = carry | (val >> 1);
		write8(addr, newVal );
		s16 res = A + newVal + PF[_CARRY];
		PF[_CARRY] = (res & 0x100);
		PF[_OVER_FLOW] = (A ^ res) & ( newVal ^ res) & 0x80;
		Update_NZ(A = (u8)res);
		TICK;

	} // Combo of ROR and ADC ( Affects Flags: V, N, Z, C )

	// Unofficial NOPs ( result in different cycle count )
	void MultiNOP( u8 code )
	{
		switch ((code & 0x0F))
		{
		case(0x04): // address ending in 0x04 is either 3 OR 4 cycle
			PC++; 
			TICK;
			if ( ( code >> 4 ) & 1 ) // if high nibble is odd, add extra dummy cycle
			{
				TICK;
			}
			break;
		case(0x0A): // ending in 0x0A is just 1 cycle
			PC += 0;
			break;
		case(0x0C): // ending in 0x0C is +2 cycles
			PC += 2; 
			TICK_2;
			if ( (code >> 4) & 1 && X != 0 ) // if high nibble is odd, add extra dummy cycle
			{
				TICK;
			}
			break;
		default: // all else is double
			PC++; 
			break;
		} // switch
		TICK;

	} // "Multi" No Operation: anything else that is not "official" NOP ( Illegal )

	void Interupt( InteruptType interupt )
	{
		if (PF[_INTERUPT_DISABLE] && interupt != InteruptType::NMI && interupt != InteruptType::BRK)
		{
			return;
		}
		TICK;
		if (interupt != InteruptType::BRK)
		{
			TICK; // BRK already performed the fetch
		}
		if (interupt != InteruptType::RESET)  // Writes on stack are inhibited on RESET
		{
			// ( 6502 Quirk ) increment PC if BRK
			if (interupt == InteruptType::BRK)
			{
				u8 dummyFetch = read8(PC++); // Holy cow...this just this is what fixed Dragon Warrior.......
			}

			// Push high and low part of address to Stack
			Push(PC >> 8);
			Push(PC & 0xFF);

			// Push Flag on Stack 
			u8 flagPush = PF.get();
			if (interupt == InteruptType::BRK)
			{
				SET_BIT(flagPush, 0x10); // Set B if BRK Interupt
			}
			else
			{
				CLEAR_BIT(flagPush, 0x10);
			}
			Push(flagPush);
		}
		else // RESET Interupt 
		{
			SP -= 3; 
			TICK_3; 
		}

		PC = read16( interuptVector[interupt] ); // Set PC to proper Interupt Address
		PF[_INTERUPT_DISABLE] = true;

		if (interupt == InteruptType::NMI)
		{
			Clear_NMI();
		}

		// (Hack) Additional 7 Ticks to combat race condition (fixes battletoads)
		if (interupt == InteruptType::NMI)
		{
			TICK_3;
			TICK_3;
			TICK;
		}

	} // Interupt()

	// Execute OpCodes, one at a time, from ROM
	void Execute()
	{	
		cpuCycle++;

		// Wait for PPU to run its steps
		if (waitCycles > 1)
		{
			waitCycles--;
			return;
		}
		nmiCycled = false;
		irqCycled = false;
		waitCycles = 0;

		// Grab Next OpCode, Increment ProgramCounter
		u8 opCode = read8(PC++);

#if DEBUG_DUMP_OPCODES
		// Capture CPU Snapshot, and write to Dump File
		if (Dev::IsCaptureRomDump())
		{
			Dev::WriteToRomDump( debugStream.str() );
		}
		std::cout << debugStream.str();
		debugStream.str(""); // Clear Stream
		DebugPrint( Dev::GetOpCodeName(opCode) );
#endif

		// Execute 6502 Operation
 		switch (opCode)
		{
		// BRK
		case(0x00): return BREAK();

		// ADC ( Add with Carry )
		case(0x69): return ADC(immediate8);
		case(0x65): return ADC(zeropage);
		case(0x75): return ADC(zeropageX);
		case(0x6D): return ADC(absolute);
		case(0x7D): return ADC(absoluteX);
		case(0x79): return ADC(absoluteY);
		case(0x61): return ADC(indirectX);
		case(0x71): return ADC(indirectY);

		// SBC ( Subtract with Carry )
		case(0xE9): return SBC(immediate8);
		case(0xE5): return SBC(zeropage);
		case(0xF5): return SBC(zeropageX);
		case(0xED): return SBC(absolute);
		case(0xFD): return SBC(absoluteX);
		case(0xF9): return SBC(absoluteY);
		case(0xE1): return SBC(indirectX);
		case(0xF1): return SBC(indirectY);

		// AND ( Bitwise & with Accumulator )
		case(0x29): return AND(immediate8);
		case(0x25): return AND(zeropage);
		case(0x35): return AND(zeropageX);
		case(0x2D): return AND(absolute);
		case(0x3D): return AND(absoluteX);
		case(0x39): return AND(absoluteY);
		case(0x21): return AND(indirectX);
		case(0x31): return AND(indirectY);

		// ORA ( Bitwise Inclusive OR with Accumulator )
		case(0x09): return ORA(immediate8);
		case(0x05): return ORA(zeropage);
		case(0x15): return ORA(zeropageX);
		case(0x0D): return ORA(absolute);
		case(0x1D): return ORA(absoluteX);
		case(0x19): return ORA(absoluteY);
		case(0x01): return ORA(indirectX);
		case(0x11): return ORA(indirectY);

		// EOR ( Bitwise XOR )
		case(0x49): return EOR(immediate8);
		case(0x45): return EOR(zeropage);
		case(0x55): return EOR(zeropageX);
		case(0x4D): return EOR(absolute);
		case(0x5D): return EOR(absoluteX);
		case(0x59): return EOR(absoluteY);
		case(0x41): return EOR(indirectX);
		case(0x51): return EOR(indirectY);

		// ASL ( Arithmetic Shift Left ) 
		case(0x0A): return ASL_A();
		case(0x06): return ASL(zeropage);
		case(0x16): return ASL(zeropageX);
		case(0x0E): return ASL(absolute);
		case(0x1E): return ASL(absoluteX_Exc);

		// BIT ( Test Bits )
		case(0x24): return BIT(zeropage);
		case(0x2C): return BIT(absolute);

		// CMP ( Compare Accumulator )
		case(0xC9): return CMP(immediate8);
		case(0xC5): return CMP(zeropage);
		case(0xD5): return CMP(zeropageX);
		case(0xCD): return CMP(absolute);
		case(0xDD): return CMP(absoluteX);
		case(0xD9): return CMP(absoluteY);
		case(0xC1): return CMP(indirectX);
		case(0xD1): return CMP(indirectY);

		// CPX ( Compare X Register )
		case(0xE0): return CPX(immediate8);
		case(0xE4): return CPX(zeropage);
		case(0xEC): return CPX(absolute);

		// CPY ( Compare X Register )
		case(0xC0): return CPY(immediate8);
		case(0xC4): return CPY(zeropage);
		case(0xCC): return CPY(absolute);

		// STA ( Store Accumulator )
		case(0x85): return STA( zeropage() );
		case(0x95): return STA( zeropageX() );
		case(0x8D): return STA( absolute() );
		case(0x9D): return STA( AbsoluteX );
		case(0x99): return STA( AbsoluteY );
		case(0x81): return STA( indirectX() );
		case(0x91): return STA( IndirectY );

		// STX ( Store X Register )
		case(0x86): return STX( zeropage() );
		case(0x96): return STX( zeropageY() );
		case(0x8E): return STX( absolute() );

		// STY ( Store Y Register )
		case(0x84): return STY( zeropage() );
		case(0x94): return STY( zeropageX() );
		case(0x8C): return STY( absolute() );

		// LDA ( Load to Accumulator )
		case(0xA9): return LDA(immediate8);
		case(0xA5): return LDA(zeropage);
		case(0xB5): return LDA(zeropageX);
		case(0xAD): return LDA(absolute);
		case(0xBD): return LDA(absoluteX);
		case(0xB9): return LDA(absoluteY);
		case(0xA1): return LDA(indirectX);
		case(0xB1): return LDA(indirectY);

		// LDX ( Load to X Register )
		case(0xA2): return LDX(immediate8);
		case(0xA6): return LDX(zeropage);
		case(0xB6): return LDX(zeropageY);
		case(0xAE): return LDX(absolute);
		case(0xBE): return LDX(absoluteY);

		// LDY ( Load to Y Register )
		case(0xA0): return LDY(immediate8);
		case(0xA4): return LDY(zeropage);
		case(0xB4): return LDY(zeropageX);
		case(0xAC): return LDY(absolute);
		case(0xBC): return LDY(absoluteX);

		// INC ( Increment Memory )
		case(0xE6): return INC(zeropage);
		case(0xF6): return INC(zeropageX);
		case(0xEE): return INC(absolute);
		case(0xFE): return INC(absoluteX_Exc);

		// DEC ( Decrement Memory )
		case(0xC6): return DEC(zeropage);
		case(0xD6): return DEC(zeropageX);
		case(0xCE): return DEC(absolute);
		case(0xDE): return DEC(absoluteX_Exc);

		// LSR ( Logical Shift Right )
		case(0x4A): return LSR_A();
		case(0x46): return LSR(zeropage);
		case(0x56): return LSR(zeropageX);
		case(0x4E): return LSR(absolute);
		case(0x5E): return LSR(absoluteX_Exc);

		// ROL ( Rotate Left )
		case(0x2A): return ROL_A();
		case(0x26): return ROL(zeropage);
		case(0x36): return ROL(zeropageX);
		case(0x2E): return ROL(absolute);
		case(0x3E): return ROL(absoluteX_Exc);

		// ROR ( Rotate Right )
		case(0x6A): return ROR_A();
		case(0x66): return ROR(zeropage);
		case(0x76): return ROR(zeropageX);
		case(0x6E): return ROR(absolute);
		case(0x7E): return ROR(absoluteX_Exc);

		// Branches
		case(0x10): return Branch(_NEGATIVE, false);	// BPL, on Plus
		case(0x30): return Branch(_NEGATIVE, true);		// BMI, on Minus
		case(0x50): return Branch(_OVER_FLOW, false);	// BVC, on Overflow Clear
		case(0x70): return Branch(_OVER_FLOW, true);	// BVS, on Overflow Set
		case(0x90): return Branch(_CARRY, false);		// BCC, on Carry Clear
		case(0xB0): return Branch(_CARRY, true);		// BCS, on Carry Set
		case(0xD0): return Branch(_ZERO, false);		// BNE, on Not Equal
		case(0xF0): return Branch(_ZERO, true);			// BEQ, on Equal

		// JMP ( Jump or Indirect Jump )
		case(0x4C): return JMP();
		case(0x6C): return JMP_INDR();

		// JSR ( Jump to Subroutine )
		case(0x20): return JSR();

		// RTS ( Return from Subroutine )
		case(0x60): return RTS();

		// RTI ( Return from Interupt )
		case(0x40): return RTI();

		// Stack Instructions
		case(0x9A): return TXS();
		case(0xBA): return TSX();
		case(0x48): return PHA();
		case(0x68): return PLA();
		case(0x08): return PHP();
		case(0x28): return PLP();

		//-------------- Register Instructions --------------//
		case(0xAA): return TAX();
		case(0x8A): return TXA();
		case(0xCA): return DEX();
		case(0xE8): return INX();
		case(0xA8): return TAY();
		case(0x98): return TYA();
		case(0x88): return DEY();
		case(0xC8): return INY();

		// Processor Flag
		case(0x18): return ClearFlag(_CARRY);
		case(0x38): return SetFlag(_CARRY);
		case(0x58): return ClearFlag(_INTERUPT_DISABLE);
		case(0x78): return SetFlag(_INTERUPT_DISABLE);
		case(0xB8): return ClearFlag(_OVER_FLOW);
		case(0xD8): return ClearFlag(_DECIMAL_MODE);
		case(0xF8): return SetFlag(_DECIMAL_MODE);

		//-------------- Unofficial OpCodes --------------//
		case(0xAF): return LAX(absolute);
		case(0xBF): return LAX(absoluteY);
		case(0xA7): return LAX(zeropage);
		case(0xB7): return LAX(zeropageY);
		case(0xA3): return LAX(indirectX);
		case(0xB3): return LAX(indirectY);

		case(0x83): return SAX(indirectX);
		case(0x87): return SAX(zeropage);
		case(0x8F): return SAX(absolute);
		case(0x97): return SAX(zeropageY);

		case(0xC7): return DCP(zeropage);
		case(0xD7): return DCP(zeropageX);
		case(0xCF): return DCP(absolute);
		case(0xDF): return DCP(absoluteX);
		case(0xDB): return DCP(absoluteY);
		case(0xC3): return DCP(indirectX);
		case(0xD3): return DCP(indirectY);

		case(0xE7): return ISB(zeropage);
		case(0xF7): return ISB(zeropageX);
		case(0xEF): return ISB(absolute);
		case(0xFF): return ISB(absoluteX);
		case(0xFB): return ISB(absoluteY);
		case(0xE3): return ISB(indirectX);
		case(0xF3): return ISB(indirectY);

		case(0x07): return SLO(zeropage);
		case(0x17): return SLO(zeropageX);
		case(0x0F): return SLO(absolute);
		case(0x1F): return SLO(absoluteX);
		case(0x1B): return SLO(absoluteY);
		case(0x03): return SLO(indirectX);
		case(0x13): return SLO(indirectY);

		case(0x27): return RLA(zeropage);
		case(0x37): return RLA(zeropageX);
		case(0x2F): return RLA(absolute);
		case(0x3F): return RLA(absoluteX);
		case(0x3B): return RLA(absoluteY);
		case(0x23): return RLA(indirectX);
		case(0x33): return RLA(indirectY);

		case(0x47): return SRE(zeropage);
		case(0x57): return SRE(zeropageX);
		case(0x4F): return SRE(absolute);
		case(0x5F): return SRE(absoluteX);
		case(0x5B): return SRE(absoluteY);
		case(0x43): return SRE(indirectX);
		case(0x53): return SRE(indirectY);

		case(0x67): return RRA(zeropage);
		case(0x77): return RRA(zeropageX);
		case(0x6F): return RRA(absolute);
		case(0x7F): return RRA(absoluteX);
		case(0x7B): return RRA(absoluteY);
		case(0x63): return RRA(indirectX);
		case(0x73): return RRA(indirectY);
		//------------------------------------------//

		// Misc
		case(0xEB): return SBC(immediate8); // *Same as Legal 0xE9
		case(0xEA): return NOP();
		default: return MultiNOP(opCode);
			
		} // switch

	} // Execute()

	//Turn on the CPU, Reset Settings
	void PowerOn()
	{
		A = 0x00;
		X = 0x00;
		Y = 0x00;
		SP = 0xFD;
		PF.set(0x34);
		memset( ram, 0x0, sizeof(ram) );

		nmiFlag = false;
		irqFlag = false;

		waitCycles	= 0;
		cpuCycle	= 0;
		timingCycle = 0;

		PC = read16( interuptVector[InteruptType::RESET] ); // Set PC to start point

	} // PowerOn()

	//Run the CPU for roughly a Frame
	void RunFrame()
	{
		timingCycle += TOTAL_CYCLES;
		while (timingCycle > 0)
		{
			PPU_EXECUTE;
			Execute();

			Cartridge::SignalCPU();

			// Check for Interupts
			if (nmiFlag && !nmiCycled)
			{
				Interupt(InteruptType::NMI);
			}
			if (irqFlag && !irqCycled)
			{
				Interupt(InteruptType::IRQ);
			}

		} // while

		// Run APU
		APU::RunFrame( elapsed() );

	} // RunFrame()

} // CPU
