#include "dev.h"

// Conntendo
#include "cpu.h"
#include "emulator.h"
#include "files.h"

// STL
#include <sstream>

namespace Dev
{
#define DUMP_FILE_NAME "OpCodesDump"

#define BATCH_DURATION_SHORT 180
#define BATCH_DURATION_LONG  360

#define FRAME_TRACK 120
#define FPS_FONT_HEIGHT 12

	StoreInfo romDump; // for storing cpu debug opcodes
	int devClock = 0;

	// Batch Variables
	bool toBatchTest = false;
	int batchTotal = 0;
	int batchCurrent = 0;
	int batchDuration = BATCH_DURATION_SHORT;
	string romBatchList[64];

	// Dev Folder Paths
	string batchFolderPath;
	string dumpFolderPath;
	string romDefaultPath;

	// Frame Rate
	DispMessage fpsMessage;
	bool bDisplayFPS = false;
	u32 frameRateList[FRAME_TRACK];
	u8 averageIter = 0;
	u32 previousTicks = 0;

	// returns whether or not Dump is happening
	bool IsCaptureRomDump()
	{
		return romDump.infoOpen;

	} // IsCaptureRomDump()

	// Write Lines to Output File
	void WriteToRomDump( string debugLine)
	{
		romDump.ignoreUntil--;
		if (romDump.ignoreUntil > 0)
		{
			return;
		}
		romDump.info += debugLine;

		// Save out to File once Lines reach limit
		romDump.lineCurrent++;
		if (romDump.lineCurrent >= romDump.lineMax)
		{
			romDump.infoOpen = false;
			romDump.info.size();
			SaveFile( romDump, DUMP_FILE_NAME);
		}

	} // WriteToRomDump()

	void SetupText(SDL_Renderer* renderer)
	{
		GetMessage()->posY = 0;
		GetMessage()->fontHeight = FPS_FONT_HEIGHT;
		GetMessage()->renderer = renderer;

	} // SetupText()

	DispMessage* GetMessage()
	{
		return &fpsMessage;

	} // GetMessage()

	void ShowMessage(string newMessage)
	{
		GetMessage()->UpdateText(newMessage);

	} // ShowMessage()

	// Get or create Paths to Emulator Folders
	void SetDirectories()
	{
		Files::CheckDirectory(&batchFolderPath, BATCH_FOLDER);
		Files::CheckDirectory(&dumpFolderPath, DUMP_FOLDER);
		Files::CheckDirectory(&romDefaultPath, DEFAULT_ROM_FOLDER);

	} // SetDirectories()

	// Save out Debug Txt Files etc
	void SaveFile( StoreInfo storeInfo, const char* fileName)
	{
		string filePath = dumpFolderPath + fileName + DUMP_EXT;
		ofstream saveState( filePath.c_str() );
		int size = storeInfo.info.size();
		saveState.write(storeInfo.info.data(), size );
		saveState.close();

	} // SaveFile()

	// Load Debug Txt Files etc
	u16* LoadFile(const char* fileName)
	{
		ifstream txtFile(fileName);

		if (!txtFile)
		{
			printf("Error TXT File Doesnt Exist \n");
			return nullptr;
		}

		// Find out TXT File size
		int lineCount = 0;
		string dummy;
		while (std::getline(txtFile, dummy))
		{
			lineCount++;
		} // while
		txtFile.clear();
		txtFile.seekg(0, ios::beg);

		string* lines = new string[lineCount]; // creates SIZE empty strings
		for (int i = 0; i < lineCount; ++i)
		{
			std::getline(txtFile, lines[i], '\n'); // read the next line into the next string
			lines[i] = lines[i].substr(0, 4); // Get Start Address
		} // for

		u16* outputArray = new u16[lineCount];
		for (int i = 0; i < lineCount; ++i)
		{
			outputArray[i] = std::stoi(lines[i], 0, HEX_NUM); // Dec or Hex

		} // for

		return outputArray;

	} // LoadFile()

	string GetTestPath(string romName)
	{
		return (romDefaultPath + romName + NES_EXT);

	} // GetTestPath()

	void DisableBatchTest()
	{
		toBatchTest = false;

	} // DisableBatchTest()

	void EnableBatchTest( bool isLongBatch )
	{
		toBatchTest = true;
		devClock = 0;
		batchDuration = (isLongBatch) ? BATCH_DURATION_LONG : BATCH_DURATION_SHORT;
		batchTotal = Files::GetListOfFiles(batchFolderPath, romBatchList);
		batchCurrent = 0;

		if (batchTotal <= 0)
		{
			Emulator::ShowMessage("Coudlnt Run Batch: Folder Empty");
		}
		BatchTestGames();

	} // EnableBatchTest()

	// Iterate through several ROMs to verify that Conntendo is still working!
	bool BatchTestGames()
	{
		if (!toBatchTest)
		{
			return false;
		}

		if (batchCurrent < batchTotal)
		{
			string romName = romBatchList[batchCurrent];
			if (romName.length() > 4)
			{
				string currentROMPath = batchFolderPath + romName;
				Emulator::RunGame(currentROMPath.c_str());
			}
			batchCurrent++;
			devClock = batchDuration;
			return true;
		}
		else // Batch Test Ended
		{
			toBatchTest = false;
			return false;
		}

	} // BatchTestGames()

	bool ToggleDisplayFPS()
	{
		return bDisplayFPS = !bDisplayFPS;

	} // ToggleDisplayFPS()

	// Frame Rate Logic
	void UpdateFrameRate()
	{
		if (!bDisplayFPS)
		{
			return;
		}
		u32 frameTime = 1000.0f / (SDL_GetTicks() - previousTicks);

		// Store FPS
		frameRateList[averageIter] = frameTime;
		averageIter++;
		averageIter %= FRAME_TRACK;

		// Calculate Average
		u32 averageFPS = 0;
		for (int i = 0; i < FRAME_TRACK; i++)
		{
			averageFPS += frameRateList[i];
		}
		averageFPS /= FRAME_TRACK;

		// Display FPS
		string fspStat = "FPS: " + to_string(averageFPS);
		ShowMessage(fspStat); 
		previousTicks = SDL_GetTicks();

	} // UpdateFrameRate()

	void RunDevClock()
	{
		Dev::UpdateFrameRate();
#if DEV_BUILD
		if (!toBatchTest)
		{
			return;
		}
		devClock--;
		if (devClock <= 0)
		{
			BatchTestGames();
		}
#endif

	} // RunDevClock()

	// Take in Hex and return opCode letters ( or just Hex if not defined yet )
	char* GetOpCodeName(u8 opCode)
	{
		switch (opCode)
		{
		case 0x00:
			return "BRK";
		case 0x69:
		case 0x65:
		case 0x75:
		case 0x6D:
		case 0x7D:
		case 0x79:
		case 0x61:
			return "ADC";
		case 0xE9:
		case 0xE5:
		case 0xF5:
		case 0xED:
		case 0xFD:
		case 0xF9:
		case 0xE1:
		case 0xF1:
			return "SBC";
		case 0x29:
		case 0x25:
		case 0x35:
		case 0x2D:
		case 0x3D:
		case 0x39:
		case 0x21:
		case 0x31:
			return "AND";
		case 0x09:
		case 0x05:
		case 0x15:
		case 0x0D:
		case 0x1D:
		case 0x19:
		case 0x01:
		case 0x11:
			return "ORA";
		case 0x49:
		case 0x45:
		case 0x55:
		case 0x4D:
		case 0x5D:
		case 0x59:
		case 0x41:
		case 0x51:
			return "EOR";
		case 0x0A:
		case 0x06:
		case 0x16:
		case 0x0E:
		case 0x1E:
			return "ASL";
		case 0x24:
		case 0x2C:
			return "BIT";
		case 0xC9:
		case 0xC5:
		case 0xD5:
		case 0xCD:
		case 0xDD:
		case 0xD9:
		case 0xC1:
		case 0xD1:
			return "CMP";
		case 0xE0:
		case 0xE4:
		case 0xEC:
			return "CPX";
		case 0xC0:
		case 0xC4:
		case 0xCC:
			return "CPY";
		case 0x85:
		case 0x95:
		case 0x8D:
		case 0x9D:
		case 0x99:
		case 0x81:
		case 0x91:
			return "STA";
		case 0x86:
		case 0x96:
		case 0x8E:
			return "STX";
		case 0x84:
		case 0x94:
		case 0x8C:
			return "STY";
		case 0xA9:
		case 0xA5:
		case 0xB5:
		case 0xAD:
		case 0xBD:
		case 0xB9:
		case 0xA1:
		case 0xB1:
			return "LDA";
		case 0xA2:
		case 0xA6:
		case 0xB6:
		case 0xAE:
		case 0xBE:
			return "LDX";
		case 0xA0:
		case 0xA4:
		case 0xB4:
		case 0xAC:
		case 0xBC:
			return "LDY";
		case 0xE6:
		case 0xF6:
		case 0xEE:
		case 0xFE:
			return "INC";
		case 0xC6:
		case 0xD6:
		case 0xCE:
		case 0xDE:
			return "DEC";
		case 0x4A:
		case 0x46:
		case 0x56:
		case 0x4E:
		case 0x5E:
			return "LSR";
		case 0x2A:
		case 0x26:
		case 0x36:
		case 0x2E:
		case 0x3E:
			return "ROL";
		case 0x6A:
		case 0x66:
		case 0x76:
		case 0x6E:
		case 0x7E:
			return "ROR";
		case 0x10:
			return "BPL";
		case 0x30:
			return "BMI";
		case 0x50:
			return "BVC";
		case 0x70:
			return "BVS";
		case 0x90:
			return "BCC";
		case 0xB0:
			return "BCS";
		case 0xD0:
			return "BNE";
		case 0xF0:
			return "BEQ";
		case 0x4C: 
		case 0x6C:
			return "JMP";
		case 0x20: 
			return "JSR";
		case 0x60: 
			return "RTS";
		case 0x40:
			return "RTI";
		case 0x9A: 
			return "TXS";
		case 0xBA:
			return "TSX";
		case 0x48:
			return "PHA";
		case 0x68:
			return "PLA";
		case 0x08:
			return "PHP";
		case 0x28:
			return "PLP";

		case 0xAA:
			return "TAX";
		case 0x8A:
			return "TXA";
		case 0xCA:
			return "DEX";
		case 0xE8:
			return "INX";
		case 0xA8:
			return "TAY";
		case 0x98:
			return "TYA";
		case 0x88:
			return "DEY";
		case 0xC8:
			return "INY";
		case 0x18:
			return "CLC";
		case 0x38:
			return "SEC";
		case 0x58:
			return "CLI";
		case 0x78:
			return "SEI";
		case 0xB8:
			return "CLV";
		case 0xD8:
			return "CLD";
		case 0xF8:
			return "SED";
		case 0xAF: 
		case 0xBF: 
		case 0xA7: 
		case 0xB7: 
		case 0xA3: 
		case 0xB3: 
			return "LAX";
		case 0x83: 
		case 0x87: 
		case 0x8F: 
		case 0x97: 
			return "SAX";
		case 0xC7: 
		case 0xD7: 
		case 0xCF: 
		case 0xDF: 
		case 0xDB: 
		case 0xC3: 
		case 0xD3: 
			return "DCP";
		case 0xE7: 
		case 0xF7: 
		case 0xEF: 
		case 0xFF: 
		case 0xFB: 
		case 0xE3: 
		case 0xF3: 
			return "ISB";
		case 0x07:
		case 0x17:
		case 0x0F:
		case 0x1F:
		case 0x1B:
		case 0x03:
		case 0x13: 
			return "SLO";
		case 0x27: 
		case 0x37: 
		case 0x2F: 
		case 0x3F: 
		case 0x3B: 
		case 0x23: 
		case 0x33: 
			return "RLA";
		case 0x47: 
		case 0x57: 
		case 0x4F: 
		case 0x5F: 
		case 0x5B: 
		case 0x43: 
		case 0x53: 
			return "SRE";
		case 0x67: 
		case 0x77: 
		case 0x6F: 
		case 0x7F: 
		case 0x7B: 
		case 0x63: 
		case 0x73: 
			return "RRA";
		case(0xEA): 
			return "NOP";
		default:
			return "*NOP";

		} // switch

	} // GetOpCodeName()

} // Dev