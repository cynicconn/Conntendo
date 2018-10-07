#include "emulator.h"

// Conntendo
#include "common.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "cartridge.h"
#include "files.h"
#include "dev.h"
#include "palette.h"
#include "joypad.h"

// Resources
#define FONT_NAME	"Sans.ttf"

// Misc
#define MASK_SCANLINE	0x3F3F3F3F
#define VOLUME_INCR		0.1

// Image Resources
#define IMAGE_BACKDROP		"backdrop"
#define IMAGE_ERROR_MAPPER	"error_mapper"

// SDL2 Screen Buffers
SDL_Texture*	gameTexture;
SDL_Texture*	filteredTexture;
SDL_Texture*	nametableTexture;
SDL_Texture*	patterntableTexture;

// Global Const Values
const int PAUSE_INTENSITY	= 0x20;
const int FONT_SIZE			= 24;

// Debug Display Options
bool bAllowNonIntegerScaling = false;
bool bEnableFiltering = false;

// Emulator Menu Visuals
DisplayImage			img_backdrop;
DisplayImage			img_error_mapper;
int backdropScroll = (HEIGHT / 2);

namespace Emulator
{
	// Const Emulator Folder Paths
	string saveFolderPath;
	string configFolderPath;
	string resourcesFolderPath;

	// For Scanlines
	u32 filterPixels[WIDTH_x2 * HEIGHT_x2]; // For drawing Scanlines ( needs to be twice as big for subpixels)
	bool drawScanlines = false;

	// Messaging
	DispMessage menuMessage;

	// EmulatorState
	bool romLoaded			= false; // if true, game will run, main logo will display if false
	bool emulatorExit		= false;
	bool emulatorPaused		= false;
	bool displayError		= false;

	// Font
	TTF_Font* theFont;

	DispMessage* GetMessage()
	{
		return &menuMessage;

	} // GetMessage() 

	// Setup Emulator
	void Setup(SDL_Renderer* renderer)
	{
		// Directories need to be set before anything else
		SetDirectories(); 

		Initialize(renderer);
		LoadResources(renderer);

		// Init Joysticks
		Joypad::InitJoysticks();

		// Init Palettes
		Palette::GeneratePalette();

		// Setup Audio
		APU::Init();

		// Setup Emulator Messages
		SetupText();
		GetMessage()->renderer = renderer; 
		Dev::SetupText(renderer);

	} // Setup()

	void LoadResources(SDL_Renderer* renderer)
	{
		// Load Images
		Files::LoadDisplayImage(renderer, IMAGE_BACKDROP, &img_backdrop);
		Files::LoadDisplayImage(renderer, IMAGE_ERROR_MAPPER, &img_error_mapper);

	} // LoadResources()

	void SetDirectories()
	{
		// Grab all paths to emulator directories
		Files::CheckDirectory(&saveFolderPath, SAVE_FOLDER);
		Files::CheckDirectory(&configFolderPath, CONFIG_FOLDER);
		Files::CheckDirectory(&resourcesFolderPath, RESOURCES_FOLDER);

#ifdef DEV_BUILD
		Dev::SetDirectories();
#endif

	} // SetDirectories()

	// Path Sharing
	string GetSavePath() { return saveFolderPath; }
	string GetConfigPath() { return configFolderPath; }
	string GetResourcesPath() { return resourcesFolderPath; }

	// Render Text to Surface
	void RenderText(DispMessage* emuMessage)
	{
		SDL_RenderCopy(emuMessage->renderer, emuMessage->shadowTexture, nullptr, &emuMessage->shadowRect);
		SDL_RenderCopy(emuMessage->renderer, emuMessage->mainTexture, nullptr, &emuMessage->mainRect);

	} // RenderText()

	// Handle Message Timer and Rendering
	bool DisplayText(DispMessage* emuMessage)
	{
		// Countdown
		if (emuMessage->timer > 0)
		{
			emuMessage->timer--;
			RenderText(emuMessage);
			return true;
		}
		return false;

	} // DisplayText()

	void SetupText()
	{
		// Initialize the Font library
		if (TTF_Init() < 0)
		{
			fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());
			SDL_Quit();
			return;
		}

		// Setup Font
		string fontPath = GetResourcesPath() + FONT_NAME;
		theFont = TTF_OpenFont(fontPath.c_str(), FONT_SIZE);

		// Exit if cannot find Font
		bool theFontResourceIsMissing = (theFont != nullptr);
		assert(theFontResourceIsMissing);

		GetMessage()->font = theFont;
		Dev::GetMessage()->font = theFont;

	} // SetupText()

	void SetLoaded(bool set)
	{
		romLoaded = set;

	} // SetLoaded()

	bool IsLoaded()
	{
		return romLoaded;

	} // IsLoaded()

	void Pause()
	{
		emulatorPaused = !emulatorPaused;
		if (emulatorPaused)
		{
			ShowMessage("PAUSED");
		}

	} // Pause()

	void PowerOn()
	{
		displayError = false;
		emulatorPaused = false;
		SetLoaded(true);
		Reset();

	} // PowerOn()

	// "Remove" the Cartridge
	void Eject()
	{
		if (Emulator::IsLoaded())
		{
			Emulator::ShowMessage("EJECTED");
			Emulator::SetLoaded(false);
		}

	} // Eject()

	// Queue Emulator to close
	void ShutDown()
	{
		emulatorExit = true;

	} // ShutDown()

	// if true, Emulator will close
	bool ToExit()
	{
		return emulatorExit;

	} // ToExit()

	// if true, emulator is running, but game is paused
	bool IsPaused()
	{
		return emulatorPaused;

	} // IsPaused()

	// Attempt to Load and Run ROM
	bool RunGame( const char* romPath)
	{
		bool gameLoaded = Cartridge::LoadROM(romPath);
		if (gameLoaded)
		{ 
			PowerOn();
		}
		else
		{
			displayError = true;
			SetLoaded(false);
		}
		return gameLoaded;

	} // RunGame()

	void ShowMessage(string newMessage)
	{
		GetMessage()->UpdateText(newMessage);

	} // ShowMessage()

	bool ToggleDrawScanlines()
	{
		memset(filterPixels, COLOR_BACKDROP_HEX, sizeof(filterPixels));
		return drawScanlines = !drawScanlines;

	} // ToggleDrawScanlines

	bool ToggleScreenFilter()
	{
		bEnableFiltering = !bEnableFiltering;
		return bEnableFiltering;
	} // ToggleScreenFilter()

	// Reset/PowerOn the NES
	void Reset()
	{
		CPU::PowerOn();
		PPU::Reset();
		APU::Reset();

	} // Reset()

	void Save()
	{
		if ( Cartridge::CreateSaveState(0) )
		{
			ShowMessage("SAVED");
		}

	} // Save()

	void Load()
	{
		if ( Cartridge::LoadSaveState(0) )
		{
			ShowMessage("LOADED");
		}

	} // Load()

	void Initialize( SDL_Renderer* renderer)
	{
		filteredTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH_x2, HEIGHT_x2);
		gameTexture		= SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

	} // Initialize()

	void DebugInitialize(SDL_Renderer* renderer, int windowType)
	{
		switch(windowType)
		{
		case 1:
			nametableTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH_x2, HEIGHT_x2);
			break;
		case 2:
			patterntableTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, WIDTH);
			break;
		} // switch

	} // DebugInitialize()

	// Limit Pixel color by percentage
	inline u32 ReduceColor(u32 oldColor, float percent)
	{
		u8 red = ((oldColor >> 16) & 0xFF) * percent;
		u8 green = ((oldColor >> 8) & 0xFF) * percent;
		u8 blue = ((oldColor >> 0) & 0xFF) * percent;
		return (red<<16) | (green<<8) | (blue<<0);

	} // ReduceColor()

	// Add Scanlines to VideoBuffer ( requires double resolution to access "sub pixels" )
	void CreateScanlines( u32* pixels )
	{
		int oI = 0;
		int oJ = 0;
		for (int i = 1; i < HEIGHT_x2; i+= 2 )
		{
			oJ = 0;
			for (int j = 0; j < WIDTH_x2; j++)
			{
				int current = (WIDTH_x2*i) + j;
				int prev	= current - WIDTH_x2;
				int grab	= (WIDTH*oI) + oJ;
				filterPixels[current] = pixels[grab];
				filterPixels[prev] = pixels[grab] & MASK_SCANLINE;
				if (j % 2 == 0) // even lines only
				{
					oJ++;
				}
			} // for
			oI++;
		} // for

	} // CreateScanlines()

	bool ToggleAllowNonIntegerScaling() 
	{
		return bAllowNonIntegerScaling = !bAllowNonIntegerScaling;

	} // ToggleAllowNonIntegerScaling()

	bool IsNonIntegerScalingAllowed()
	{
		return bAllowNonIntegerScaling;

	} // IsNonIntegerScalingAllowed()

	void AdjustVolume(bool louder)
	{
		float adjust = (louder) ? VOLUME_INCR : -VOLUME_INCR;
		APU::AdjustVolume(adjust);
		ShowMessage(APU::PrintVolume());

	} // AdjustVolume()

	// Render the Emulator
	void RenderScreen(SDL_Renderer* renderer, SDL_Renderer* ntRenderer, SDL_Renderer* ptRenderer)
	{
		// Clear Screen to specified Color
		SDL_RenderClear(renderer);
		SDL_RenderClear(ntRenderer);
		SDL_RenderClear(ptRenderer);

		if (displayError)
		{
			// Draw Mapper Error
			SDL_Rect messageRect = { 0, 0, WIDTH, HEIGHT };
			SDL_RenderCopy(renderer, img_error_mapper.img, nullptr, &messageRect);
		}
		else if (!IsLoaded())
		{
			// Draw Backdrop
			SDL_Rect backdropRect = { 0, -backdropScroll, WIDTH, HEIGHT }; // Bottom Half of screen
			SDL_RenderCopy(renderer, img_backdrop.img, nullptr, &backdropRect);
			backdropScroll = (backdropScroll <= 0) ? 0 : (backdropScroll - 1);
		}
		else
		{
			// Draw the NES Screen
			if (IsLoaded())
			{
				CopyToRenderer(renderer, ntRenderer, ptRenderer);
			}
		}

		// Display Emulator Messages
		DisplayText(GetMessage());
		DisplayText(Dev::GetMessage());

		// Ready to Render
		SDL_RenderPresent(renderer);
		SDL_RenderPresent(ntRenderer);
		SDL_RenderPresent(ptRenderer);

	} // RenderScreen()

	// Send the rendered frame to the GUI 
	void NewFrame( u32* pixels )
	{
		if (drawScanlines)
		{
			CreateScanlines(pixels);
			SDL_UpdateTexture(filteredTexture, nullptr, filterPixels, WIDTH_x2 * sizeof(u32));
		}
		else
		{
			SDL_UpdateTexture(gameTexture, nullptr, pixels, WIDTH * sizeof(u32));
		}

	} // NewFrame()

	// Send the rendered frame to the GUI 
	void NewDebugFrame(u32* pixels, bool isNametable )
	{
		if (isNametable)
		{
			SDL_UpdateTexture(nametableTexture, nullptr, pixels, WIDTH_x2 * sizeof(u32));
		}
		else // Pattern Table
		{
			SDL_UpdateTexture(patterntableTexture, nullptr, pixels, WIDTH * sizeof(u32));
		}

	} // NewDebugFrame()

	void CopyToRenderer( SDL_Renderer* renderer, SDL_Renderer* ntRenderer, SDL_Renderer* ptRenderer )
	{
		SDL_Texture* texture = (drawScanlines) ? filteredTexture : gameTexture;

		// Tint Screen when Paused
		int colorMod = IsPaused() ? PAUSE_INTENSITY : 0xFF;
		SDL_SetTextureColorMod(texture, colorMod, colorMod, colorMod*4 );

		// Copy Screens to be Rendered
		SDL_RenderCopy(renderer, texture, nullptr, nullptr);
		SDL_RenderCopy( ntRenderer, nametableTexture, nullptr, nullptr);
		SDL_RenderCopy(ptRenderer, patterntableTexture, nullptr, nullptr);

	} // CopyToRenderer()

} // Emulator