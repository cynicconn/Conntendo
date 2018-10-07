#include "files.h"

// Conntendo 
#include "joypad.h"
#include "cpu.h"

// Windows
#include <assert.h>
#include <Windows.h>

// Compression
#include "zlib.h"

// Wrapper for Sys Function
#define TheCurrentDirectory _getcwd

// File and Folder Consts
const int PATH_SIZE = 256; // Arbitrary Size

namespace Files
{
	// Get ROM from Path and return contents
	u8* ReadROMFile(const char* romPath)
	{
		FILE* romFile;

		// Open ROM File from specified path
		errno_t err = fopen_s(&romFile, romPath, READ_BINARY); // using instead of fopen() because Microsoft complains...

		// Exit if ROM File was not retrieved
		if (romFile == nullptr)
		{
			return nullptr;
		}

		// Find out ROM File size
		fseek(romFile, 0, SEEK_END);
		int size = ftell(romFile);
		fseek(romFile, 0, SEEK_SET);

		// Copy ROM File contents into u8* memory block
		u8* outROM = new u8[size];
		fread(outROM, size, 1, romFile);
		fclose(romFile);

		return outROM;

	} // ReadROMFile()

	// Get relative emulator Path to desired Folder
	void GetFolderPath(string* theFolderPath, const char* folderName)
	{
		char* tempFolderPath = new char[PATH_SIZE];
		TheCurrentDirectory(tempFolderPath, PATH_SIZE);
		strcat(tempFolderPath, folderName);
		*theFolderPath = tempFolderPath; // copy over Path to ref
		delete[] tempFolderPath;

	} // GetFolderPath()

	// Check if Emualtor Folder exists: if not, create it
	void CheckDirectory(string* theFolderPath, char* folderName)
	{
		GetFolderPath(theFolderPath, folderName);

		// Create folder if it doesnt exist
		DWORD attribs = ::GetFileAttributesA(theFolderPath->c_str());
		if (attribs == INVALID_FILE_ATTRIBUTES)
		{
			CreateDirectory(theFolderPath->c_str(), nullptr);
		}

	} // CheckDirectory()

	// Return list of all files in a directory
	int GetListOfFiles( string folderPath, string* fileList )
	{
		string pattern(folderPath);
		pattern.append("\\*");
		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile(pattern.c_str(), &findData);
		int iter = 0;
		if (hFind != INVALID_HANDLE_VALUE) 
		{
			while (FindNextFile(hFind, &findData) != 0 && iter < 64)
			{
				string potentialFile = findData.cFileName;
				if (potentialFile.length() > 4)
				{
					fileList[iter] = findData.cFileName;
					iter++;
				}
			} // while
			FindClose(hFind);
		}
		return iter;

	} // GetListOfFiles()

	// Load Image from path and put it through SDL Format
	void LoadDisplayImage(SDL_Renderer* renderer, const char* imgName, DisplayImage* imageTo)
	{
		string imagePath = Emulator::GetResourcesPath() + imgName + IMAGE_EXT;

		SDL_Surface* imgSurface = nullptr;
		imgSurface = IMG_Load(imagePath.c_str());

		// End Program Image could not be found
		bool theImageResourceIsMissing = (imgSurface != nullptr);
		assert(theImageResourceIsMissing);

		imageTo->img = SDL_CreateTextureFromSurface(renderer, imgSurface);
		SDL_FreeSurface(imgSurface);
		SDL_QueryTexture(imageTo->img, nullptr, nullptr, &imageTo->width, &imageTo->height);

	} // LoadDisplayImage()

	// Create a Conntendo File
	bool SaveConnFile(string filePath, FileType type)
	{
		// Save based on FileType (Currently only Joypad)
		Joypad::InputConfig saveInput[4];
		Joypad::GetSaveInputConfig(saveInput);

#if USE_COMPRESSION
		uLongf dataSize = sizeof(saveInput);
		Bytef* testCompress = (Bytef*)&saveInput;
		uLongf sizeDataCompressed = (dataSize * 1.1f) + 12;
		Bytef* dataCompressed = (Bytef*)malloc(sizeDataCompressed);
		int z_result = compress(dataCompressed, &sizeDataCompressed, testCompress, dataSize);

		FILE* out = fopen(filePath.c_str(), WRITE_BINARY);
		if (out == nullptr)
		{
			return false;
		}
		fwrite(dataCompressed, sizeDataCompressed, 1, out);
		fclose(out);
		out = nullptr;
#else
		ofstream saveState(filePath.c_str(), ios::binary);
		saveState.write((char*)&saveInput, sizeof(saveInput));
		saveState.close();
#endif
		return true;

	} // SaveConnFile()

	// Load a Conntendo File ( currently hardcoded for Joypad Input )
	bool LoadConnFile(string filePath, FileType type)
	{
		// Load based on FileType (Currently only Joypad)
		Joypad::InputConfig loadInput[4];

#if USE_COMPRESSION
		FILE* readFile = fopen(filePath.c_str(), READ_BINARY);
		if (readFile == nullptr)
		{
			return false;
		}

		// Get Size of File
		fseek(readFile, 0, SEEK_END);
		uLongf fileLength = ftell(readFile);
		rewind(readFile);

		Bytef* dataReadInCompressed = (Bytef*)malloc(fileLength);
		fread(dataReadInCompressed, fileLength, 1, readFile);

		// Close File
		fclose(readFile);
		readFile = nullptr;

		uLongf sizeDataUncompressed = sizeof(Joypad::InputConfig) * 4;
		Bytef* dataUncompressed = (Bytef*)malloc(sizeDataUncompressed);
		uLong sizeDataCompressed = fileLength;
		int z_result = uncompress(dataUncompressed, &sizeDataUncompressed, dataReadInCompressed, sizeDataCompressed);
		memcpy((char*)&loadInput, (char*)dataUncompressed, sizeof(loadInput));

		Joypad::LoadInputConfig(loadInput);
#else
		ifstream saveRead(filePath.c_str(), ios::binary);
		if (saveRead.good())
		{
			saveRead.read((char*)&loadInput, sizeof(loadInput));
			saveRead.close();
			Joypad::LoadInputConfig(loadInput[0], loadInput[1], loadInput[2], loadInput[3]);
		}
#endif
		return true;

	} // LoadConnFile()

	bool CreateSaveState(string filePath)
	{
		CPU::SaveData saveData = CPU::GrabSaveData();

#if USE_COMPRESSION
		uLongf dataSize = sizeof(saveData);
		Bytef* testCompress = (Bytef*)&saveData;
		uLongf sizeDataCompressed = (dataSize * 1.1f) + 12;
		Bytef* dataCompressed = (Bytef*)malloc(sizeDataCompressed);
		int z_result = compress(dataCompressed, &sizeDataCompressed, testCompress, dataSize);

		FILE* out = fopen(filePath.c_str(), WRITE_BINARY);
		if (out == nullptr)
		{
			return false;
		}
		fwrite(dataCompressed, sizeDataCompressed, 1, out);
		fclose(out);
		out = nullptr;
#else
		ofstream saveState(filePath.c_str(), ios::binary);
		saveState.write((char*)&saveData, sizeof(saveData));
		saveState.close();
#endif
		return true;

	} // CreateSaveState()

	bool LoadSaveState(string filePath)
	{
		CPU::SaveData testData;

#if USE_COMPRESSION
		FILE* readFile = fopen(filePath.c_str(), READ_BINARY);
		if (readFile == nullptr)
		{
			return false;
		}

		// Get size of File
		fseek(readFile, 0, SEEK_END);
		uLongf fileLength = ftell(readFile);
		rewind(readFile);

		Bytef* dataReadInCompressed = (Bytef*)malloc(fileLength);
		fread(dataReadInCompressed, fileLength, 1, readFile);

		// Close File
		fclose(readFile);
		readFile = nullptr;

		uLongf sizeDataUncompressed = sizeof(CPU::SaveData);
		Bytef* dataUncompressed = (Bytef*)malloc(sizeDataUncompressed);
		uLong sizeDataCompressed = fileLength;
		int z_result = uncompress(dataUncompressed, &sizeDataUncompressed, dataReadInCompressed, sizeDataCompressed);
		memcpy((char*)&testData, (char*)dataUncompressed, sizeof(testData));

		CPU::LoadSaveData(testData);
#else
		ifstream saveRead(filePath.c_str(), ios::binary);
		if (saveRead.good())
		{
			saveRead.read((char*)&testData, sizeof(testData));
			saveRead.close();
			CPU::LoadSaveData(testData);
		}
#endif
		return true;

	} // LoadSaveState()

} // Files