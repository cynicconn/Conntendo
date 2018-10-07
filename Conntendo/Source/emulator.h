#pragma once
//----------------------------------------------------------------//
// This runs the Frontend and Menu functions of the Emulator
// Screen drawing is also handled here
//----------------------------------------------------------------//

#include "common.h"

// SDL
#include "SDL_image.h"
#include "SDL_ttf.h"

// NES Resolution
const u32 WIDTH			= 256;
const u32 HEIGHT		= 240;

// x2 Resolution
const u32 WIDTH_x2		= WIDTH*2;
const u32 HEIGHT_x2		= HEIGHT*2;

// Window Size Scale
const u8 SCALE			= 3;
const u8 DEBUG_SCALE	= 2;

// To Ensure WinForm can see entire Game Window
const u8 PADDING		= 16; 

// Display Messages
const SDL_Color WHITE{ 0xB6,0xDA,0xFF,0xFF };
const SDL_Color BLACK{ 0,0,0,0xFF };
const int		MESSAGE_DURATION = 60;

// Emulator Info
#define VERSION_NUMBER "2.0"
#define EMULATOR_NAME  "Conntendo"

// For displaying debug and emulator messages in the game window
struct DispMessage
{
	// Main Text
	SDL_Surface*	mainSurface;
	SDL_Texture*	mainTexture;
	SDL_Rect		mainRect;

	// Shadow Backdrop ( for popping text )
	SDL_Surface*	shadowSurface;
	SDL_Texture*	shadowTexture;
	SDL_Rect		shadowRect;

	SDL_Renderer*	renderer;
	TTF_Font*		font;
	int				fontHeight;

	// Positioning
	int posX;
	int posY;

	int timer;
	char* text;
	int length;

	// Default Constructor
	DispMessage()
	{
		text		= new char[64];
		timer		= 0;
		fontHeight	= 16;
		posX		= 8;
		posY		= HEIGHT - 24;
	};

	void UpdateText(string newMessage)
	{
		if (font == nullptr)
		{
			return;
		}
		timer = MESSAGE_DURATION;
		strcpy(text, newMessage.c_str());
		length = newMessage.length();

		mainSurface = TTF_RenderText_Solid(font, text, WHITE);
		mainTexture = SDL_CreateTextureFromSurface(renderer, mainSurface);

		shadowSurface = TTF_RenderText_Solid(font, text, BLACK);
		shadowTexture = SDL_CreateTextureFromSurface(renderer, shadowSurface);

		mainRect = SDL_Rect // x, y, w, h
		{
			posX,
			posY,
			(fontHeight/2) * length,
			fontHeight
		};

		// Offset to create text shadow 
		shadowRect = mainRect;
		shadowRect.x -= 1;
		shadowRect.y += 1;

	} // UpdateText()

}; // DispMessage 

namespace Emulator
{
	// Setup
	void Setup(SDL_Renderer* renderer);
	void LoadResources(SDL_Renderer* renderer);
	void SetDirectories();

	// Pause and Run
	void PowerOn();
	void Eject();
	void ShutDown();
	bool ToExit();
	bool RunGame( const char* romPath);
	void Reset();
	void Pause();
	bool IsPaused();

	// ROM Loading
	void SetLoaded( bool set );
	bool IsLoaded();

	// Screen Rendering
	void RenderScreen(SDL_Renderer* renderer, SDL_Renderer* ntRenderer, SDL_Renderer* ptRenderer);
	void NewFrame(u32* pixels);
	void NewDebugFrame(u32* pixels, bool isNametable );
	void CopyToRenderer(SDL_Renderer* renderer, SDL_Renderer* ntRenderer, SDL_Renderer* ptRenderer);
	bool ToggleDrawScanlines();

	void Initialize(SDL_Renderer* renderer);
	void DebugInitialize(SDL_Renderer* renderer, int windowType);

	// Debug Display Functions
	bool ToggleAllowNonIntegerScaling();
	bool IsNonIntegerScalingAllowed();
	bool ToggleScreenFilter();

	// Emulator Path Sharing
	string GetSavePath();
	string GetConfigPath();
	string GetResourcesPath();

	// Message Functions
	void SetupText();
	bool DisplayText(DispMessage* emuMessage);
	DispMessage* GetMessage();
	void ShowMessage(string newMessage);

	// Save Functions
	void Save();
	void Load();

	// Misc
	inline string GetVersionNumber() { return VERSION_NUMBER; }; // should VerNum be represented as string or double?
	inline string GetEmulatorTitle() { return EMULATOR_NAME + ("  " + GetVersionNumber() ); }
	void AdjustVolume(bool louder);

} // Emulator
