#pragma once
//----------------------------------------------------------------//
// For Testing and Debugging the Emulator
//----------------------------------------------------------------//

// Conntendo
#include "common.h"
#include "emulator.h"

#define MAX_LINES_DEFAULT 75600

namespace Dev
{
	// For storing info to be written to Dump file
	struct StoreInfo
	{
		string info;		// string that will be written to file
		int lineMax;		// Max number of lines before dump file is saved out
		int lineCurrent;	// current "line" reading
		int ignoreUntil;	// do not write until after this line number
		bool infoOpen;		// if true, can be written to

		// Default Values
		StoreInfo() 
		{
			lineCurrent		= 0;
			lineMax			= MAX_LINES_DEFAULT;
			infoOpen		= true;
			ignoreUntil		= 0;
		}

	}; // StoreInfo

	// Initialize
	void SetDirectories();

	// Frame Rate
	bool ToggleDisplayFPS();
	void UpdateFrameRate();

	// Dev Messaging
	void SetupText(SDL_Renderer* renderer);
	DispMessage* GetMessage();
	void ShowMessage(string newMessage);

	// Save and Load Txt Files
	void SaveFile(StoreInfo storeInfo, const char* fileName);
	u16* LoadFile(const char* fileName);

	// ROM Dump Functions 
	void WriteToRomDump(string debugLine);
	bool IsCaptureRomDump();
	char* GetOpCodeName(u8 opCode);

	// Test ROM Functions
	string GetTestPath(string romName);

	// Batch Test Functions
	void DisableBatchTest();
	void EnableBatchTest(bool isLongBatch);
	bool BatchTestGames();
	void RunDevClock();

} // Dev