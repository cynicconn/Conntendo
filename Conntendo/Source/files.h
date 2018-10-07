#pragma once
//----------------------------------------------------------------//
// For file and folder Management
// Getting and setting paths for emulator
// Reading and writing functions
//----------------------------------------------------------------//

// Conntendo
#include "common.h"
#include "emulator.h"

// SDL2
#include "SDL_image.h"

// Conntendo Folders
#define SAVE_FOLDER			"/Savestates/"
#define CONFIG_FOLDER		"/Config/"
#define RESOURCES_FOLDER	"/Resources/"
#define BATCH_FOLDER		"/Batch/"
#define DUMP_FOLDER			"/Dump/"
#define DEFAULT_ROM_FOLDER	"/Roms/"

// Reading/Writing Files
#define READ_BINARY  "rb"
#define WRITE_BINARY "wb"

// Stores Texture and cached Texture info 
struct DisplayImage
{
	SDL_Texture* img;
	int			 width;
	int			 height;

}; // DisplayImage

namespace Files
{
	enum FileType
	{
		Joypad,
		Savestate

	}; // FileType

	// Directory Functions
	void GetFolderPath(string* theFolderPath, const char* folderName);
	void CheckDirectory(string* theFolderPath, char* folderName);
	int GetListOfFiles(string folderPat, string* fileListh);

	// ROM Functions
	u8* ReadROMFile(const char* romPath);

	// Images
	void LoadDisplayImage(SDL_Renderer* renderer, const char* imgName, DisplayImage* imageTo);

	// Savestates
	bool CreateSaveState(string filePath);
	bool LoadSaveState(string filePath);

	// Conntendo File Functions ( not generic currently )
	bool SaveConnFile(string filePath, FileType type );
	bool LoadConnFile(string filePath, FileType type );

} // Files