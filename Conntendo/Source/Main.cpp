// STL
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <vector> 

// Conntendo
#include "apu.h"
#include "cpu.h"
#include "emulator.h"
#include "cartridge.h"
#include "joypad.h"
#include "palette.h"
#include "dev.h"
#include "files.h"

// SDL
#include "SDL_syswm.h"
#include "SDL_ttf.h"

// C# CLR
#include "ConnForm.h"
#include <vcclr.h>
using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

// Filters
#define UNFILTERED "nearest"
#define FILTERED "bilinear"

// Global UI
const int TOOLBAR_HEIGHT = 32;
vector<SDL_Rect>		displayBounds;

// Forward Declarations
void options_ButtonClick(Object^ sender, EventArgs^ e);
void AdjustGameWindow();

// WinForm
gcroot<Conntendo::ConnForm^> menuForm;
gcroot<Conntendo::ConnForm^> optionForm;
HWND optionWinHandle;

// WinForm Menus
gcroot<ToolStripMenuItem^> menu_NES;
gcroot<ToolStripMenuItem^> menu_Options;
gcroot<ToolStripMenuItem^> menu_Audio;

// WinForm SubMenus
gcroot<ToolStripMenuItem^>	subMenu_DebugDisplay;
gcroot<ToolStripMenuItem^>	subMenu_DebugSpeed;
gcroot<ToolStripMenuItem^>	subMenu_Palette;
gcroot<ToolStripItem^>		subMenu_DebugSpeedPrev;
gcroot<ToolStripItem^>		subMenu_PalettePrev;

// WinForm Toolbar
gcroot<ToolStripContainer^> toolStripContainer;
gcroot<ToolStrip^> toolStrip;
HWND winHandle;
HWND connHandle;

// WinForm Label Input
gcroot<Label^> labelInput;

enum MENU_BUTTON
{
	NES			= 0,
	Open		= 1,
	Save		= 2,
	Load		= 3,
	Debug		= 4,
	Input		= 5,
	Audio		= 6,
	Test		= 7,
	Test2		= 8,
	Batch		= 9
};

enum MENU_NES
{
	Reset = 0,
	Pause = 1,
	Eject = 2,
	Exit  = 3,
};

enum MENU_OPTIONS
{
	DisplayFPS				= 0,
	NametableViewer			= 1,
	PatternTable			= 2,
	DrawScanlines			= 3,
	AllowNonIntegerScale	= 4,
	ScreenFilter			= 5
};

enum MENU_DEBUGDISPLAY
{
	HighlightSprites	= 0,
	DisableBackground	= 1,
	DisableSpriteOffset = 2,
	DisableSpriteAlpha	= 3
};

enum MENU_SPEED
{
	Quarter		= 0,
	Half		= 1,
	Normal		= 2,
	TimeNHalf	= 3,
	Double		= 4
};

enum MENU_AUDIO
{
	Increase		= 0,
	Decrease		= 1,
	Mute			= 2,
	DisablePulseA	= 3,
	DisablePulseB	= 4,
	DisableTriangle = 5,
	DisableNoise	= 6,
	DisableSample	= 7
};

enum EMULATOR_STATE
{
	Dormant,
	Running,
	Opening,
	DisplayError,
	Closing
};

struct InputUI
{
	int offset;
	int topPos;
	InputUI() {};

	void Init(int nOff, int nTop)
	{
		offset = nOff;
		topPos = nTop;
	} // Init()

	void SpawnButton(Button^ button, int off, int top)
	{
		button->Top = top;
		button->Left = 16;
		button->AutoSize = true;
		button->BackColor = COLOR_IDLE;
		button->Click += gcnew System::EventHandler(&options_ButtonClick);
		button->Name = Convert::ToString(off);
		optionForm->Controls->Add(button);
	} // SpawnButton()

}; // InputUI

// Wrapper for SDL2 Window Info
struct ConnScreen
{
	SDL_Window*		window;
	SDL_Renderer*	renderer;
	int				windowID;
	bool			hasRenderer;
	int				offset;
	int				type;
	string			name;

	ConnScreen()
	{
		window		= nullptr;
		renderer	= nullptr;
		windowID	= -1;
		hasRenderer = false;
		offset		= 0;
		name		= "Default";
		type		= 0;
	}

	ConnScreen(int nOffset, string nName, int nType)
	{
		window		= nullptr;
		renderer	= nullptr;
		windowID	= -1;
		offset		= nOffset;
		name		= nName;
		type		= nType;
	}

	// Instantiate ConnScreen
	void Spawn()
	{
		if (window != nullptr)
		{
			return;
		}

		// Setup Debug Video
		window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH*DEBUG_SCALE, HEIGHT*DEBUG_SCALE, 0);
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		SDL_RenderSetLogicalSize(renderer, WIDTH_x2, HEIGHT_x2);
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

		// Get Window ID
		windowID = SDL_GetWindowID(window);
		int xOffset = 100 + (offset * 400);
		int yOffset = 200 + (offset * 200);

		// Set Debug Window Setting
		SDL_SetWindowResizable(window, SDL_TRUE);
		SDL_SetWindowPosition(window, displayBounds[1].x + xOffset, displayBounds[1].y + yOffset);

		// Brute Force Initialize All
		Emulator::DebugInitialize(renderer, type);

	} // Spawn()

	// Safely close SDL2 Windows
	void Destroy()
	{
		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
			window = nullptr;
		}
		if (renderer != nullptr)
		{
			SDL_DestroyRenderer(renderer);
			renderer = nullptr;
		}

	} // Destroy()

}; // ConnScreen

// The Emulator State
bool	runEmulator		= true;
bool	openingROM		= false;
char*	theRomPath;

// The Conntendo SDL2 Windows
ConnScreen	mainScreen		= ConnScreen(0, Emulator::GetEmulatorTitle(), 0);
ConnScreen	ntScreen		= ConnScreen(0, "NameTable Viewer", 1);
ConnScreen	ptScreen		= ConnScreen(1, "Pattern Table Viewer", 2);
ConnScreen	inputScreen		= ConnScreen(0, "Customize Input", 3);

// Open up OS DialogBox for choosing ROM File
void OpenDialog( char* getPath )
{
	OPENFILENAME ofn;
	ZeroMemory( &ofn, sizeof(ofn) );
	ofn.lStructSize		= sizeof(ofn);
	ofn.hwndOwner		= nullptr;

	// Set WildCards
	ofn.lpstrInitialDir		= "C:/";
	ofn.lpstrFilter			= "NES ROM Files\0*.nes\0 Any File\0*.*\0";

	ofn.lpstrFile	= getPath;
	ofn.nMaxFile	= MAX_PATH;
	ofn.lpstrTitle	= "Select a ROM";
	ofn.Flags		= OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

	// Open DialogBox
	bool dialogOpened = GetOpenFileNameA(&ofn);

#if DEV_BUILD
	string openedMessage = (dialogOpened) ? "OPENED" : "Failed to open ROM";
	Emulator::ShowMessage(openedMessage);
#endif

} // OpenDialog()

// Open Dialog and Load game that users selects
void OpenROM()
{
	// Exit if Dialog already opened
	if (openingROM)
	{
		return;
	}

	// Check for Joypad Changes
	Joypad::InitJoysticks();

	// Open Dialog Window to select ROM
	openingROM	= true;
	theRomPath	= new char[MAX_PATH];
	ZeroMemory(theRomPath, sizeof(theRomPath));
	OpenDialog(theRomPath);
	if (strlen(theRomPath) == 0)
	{
		openingROM = false;
		return;
	}

	// Attempt to run ROM grabbed
	Emulator::RunGame(theRomPath);
	openingROM		= false;

} // OpenROM()

// Safely Exit Conntendo
void CloseEmulator()
{
	// Destroy All Windows
	mainScreen.Destroy();
	ntScreen.Destroy();
	ptScreen.Destroy();
	inputScreen.Destroy();

	runEmulator = false;
	Application::Exit();

} // CloseEmulator()

// Monitor Window Input Events
int ProcessWindowEvents()
{
	int done = FALSE;
	SDL_Event winEvent;

	// Check for events
	while ( SDL_PollEvent(&winEvent) )
	{
		if (winEvent.window.windowID == mainScreen.windowID)
		{
			switch (winEvent.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
			case SDL_QUIT:
				CloseEmulator();
				break;
			} // switch
		} 
		else if (winEvent.window.windowID == ntScreen.windowID)
		{
			switch (winEvent.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
			case SDL_QUIT:
				ntScreen.Destroy();
				break;
			} // switch
		}
		else if (winEvent.window.windowID == ptScreen.windowID)
		{
			switch (winEvent.window.event)
			{
			case SDL_WINDOWEVENT_CLOSE:
			case SDL_QUIT:
				ptScreen.Destroy();
				break;
			} // switch
		}
	} // while

	return done;

} // ProcessWindowEvents()

// Set to either Nearest Neighbor or Bilinear Filtered
bool SetScreenFilter()
{
	bool toFilter = Emulator::ToggleScreenFilter();
	if (toFilter)
	{
		SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, FILTERED, SDL_HintPriority::SDL_HINT_OVERRIDE);
	}
	else
	{
		SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, UNFILTERED, SDL_HintPriority::SDL_HINT_OVERRIDE);
	}

	// Window needs to be Recreated for Filter to take Effect
	// TODO

	return toFilter;

} // SetScreenFilter()

// CLosing Custom Input Menu
void options_ClosedClick(Object^ sender, FormClosingEventArgs^ e)
{
	Joypad::SetRecordMode(Joypad::RecordMode::None, 0);
	inputScreen.Destroy();

} // options_ClosedClick()

// Custom Input Menu
void options_ButtonClick(Object^ sender, EventArgs^ e)
{
	Button^ pressedButton = static_cast<Button^>(sender);
	int nameID = Convert::ToInt16(pressedButton->Name);

	labelInput->Show();
	labelInput->Top = pressedButton->Top;

	switch (nameID)
	{
	case 0:
		Joypad::SetRecordMode(Joypad::RecordMode::Keyboard,0);
		break;
	case 1:
		Joypad::SetRecordMode(Joypad::RecordMode::Joypad,0);
		break;
	case 2:
		Joypad::SetRecordMode(Joypad::RecordMode::Keyboard,1);
		break;
	case 3:
		Joypad::SetRecordMode(Joypad::RecordMode::Joypad,1);
		break;
	case 4:
		Emulator::ShowMessage("Input Set to Default");
		Joypad::DefaultAll();
		Joypad::SaveConfig();
		break;
	} // switch

} // options_ButtonClick()

void SpawnOptionsWindow()
{
	const int OPTION_WIDTH = WIDTH * SCALE / 3.0f;
	const int OPTION_HEIGHT = HEIGHT * SCALE / 2.25f;

	// Setup Option Window
	if ( optionForm.operator->() == nullptr || optionForm->IsDisposed )
	{
		optionForm = gcnew Conntendo::ConnForm;
		optionForm->Width = OPTION_WIDTH;
		optionForm->Height = OPTION_HEIGHT;

		optionForm->FormBorderStyle = FormBorderStyle::FixedDialog;
		optionForm->MaximizeBox = false;
		optionForm->MinimizeBox = false;

		optionForm->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(&options_ClosedClick);

		Font^ titleFont = gcnew Font("Sans", 16 );

		// Input Specific
		labelInput = gcnew Label();
		labelInput->Text = "Set Button: ";
		labelInput->TextAlign = ContentAlignment::BottomLeft;
		labelInput->Width	= 200;
		labelInput->Top		= 208;
		labelInput->Left	= 108;
		optionForm->Controls->Add(labelInput);

		cli::array<Button^>^ optionButtons = gcnew cli::array<Button^>(5);

		InputUI inputUI[2];
		inputUI[0].Init(0,8);
		inputUI[1].Init(2,128);

		for (int i = 0; i < 2; i++)
		{
			InputUI ui = inputUI[i];
			int off = ui.offset;

			// Title
			Label^ titleLabel = gcnew Label();
			titleLabel->Font = titleFont;
			titleLabel->Text = "Player " + Convert::ToString(i+1);
			titleLabel->TextAlign = ContentAlignment::BottomLeft;
			titleLabel->Top  = ui.topPos;
			titleLabel->Left = 16;
			titleLabel->Show();
			optionForm->Controls->Add(titleLabel);

			// Keyboard Button
			optionButtons[off] = gcnew System::Windows::Forms::Button();
			ui.SpawnButton( optionButtons[off], off, ui.topPos+32);
			optionButtons[off]->Text = "Keyboard";

			// Joypad Button
			optionButtons[off+1] = gcnew System::Windows::Forms::Button();
			ui.SpawnButton( optionButtons[off+1], off+1, ui.topPos+64);
			optionButtons[off+1]->Text = "Joypad";

		} // for

		// Reset All Button
		int rI = 4;
		optionButtons[rI] = gcnew System::Windows::Forms::Button();
		optionButtons[rI]->Top = 240;
		optionButtons[rI]->Left = 16;
		optionButtons[rI]->AutoSize = true;
		optionButtons[rI]->BackColor = COLOR_IDLE;
		optionButtons[rI]->Click += gcnew System::EventHandler(&options_ButtonClick);
		optionButtons[rI]->Name = Convert::ToString(rI);
		optionForm->Controls->Add(optionButtons[rI]);
		optionButtons[rI]->Text = "Reset All";
		
	}
	optionForm->Show();

	// Spawn SDL Window for capturing input (no renderer)
	if (inputScreen.window == nullptr)
	{
		inputScreen.window = SDL_CreateWindow("Option Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 100, 100, SDL_WINDOW_HIDDEN);
				
		HWND formHandle = static_cast<HWND>( optionForm->Handle.ToPointer() );

		// Embed SDL Window inside WinForm Window 
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version); 
		SDL_GetWindowWMInfo(inputScreen.window, &info);
		optionWinHandle = info.info.win.window;
		SetParent(optionWinHandle, formHandle);
	}

} // SpawnOptionsWindow()

// For Setting Keyboard/Joypad Configuration
void CustomizingInput( const u8* state )
{
	if ( Joypad::IsRecordingInput() )
	{
		labelInput->Text = gcnew String(Joypad::GetCurrentRecording().c_str());
		BringWindowToTop(optionWinHandle); // Force SDL Option Window to have focus to recieve input (hacky)
		Joypad::AssignPlayerInput(state);
	}
	else if (labelInput.operator->() != nullptr )
	{
		labelInput->Hide();
	}

} // CustomizingInput()

// Detect Size of Computer Monitors
void DetectMonitors()
{
	int displays = SDL_GetNumVideoDisplays();
	if (displays > 1)
	{
		// Get display bounds for all displays
		for (int i = 0; i < displays; i++)
		{
			displayBounds.push_back(SDL_Rect());
			SDL_GetDisplayBounds(i, &displayBounds.back());
		} // for
	}

} // DetectMonitors()

int ProcessInputsAndEvents()
{
	int done = ProcessWindowEvents();

	const u8* state = SDL_GetKeyboardState(nullptr);

	CustomizingInput(state);
	u8 joyCommand = Joypad::ProcessPlayerInput(state);

	switch (joyCommand)
	{
	case 4:
	{
		ntScreen.Spawn();
		break;
	}
	case 5:
	{
		ntScreen.Destroy();
		break;
	}
	default:
		break;
	} // switch

	return done;

} // ProcessInputsAndEvents()

// The Main Emulator Loop
void RunEmulator()
{
	while (runEmulator)
	{
		if ( Emulator::IsLoaded() && !Emulator::IsPaused() )
		{
			CPU::RunFrame(); // Run Emulation
		}
		Emulator::RenderScreen(mainScreen.renderer, ntScreen.renderer, ptScreen.renderer);
		ProcessInputsAndEvents();
		Dev::RunDevClock();
		if (Emulator::ToExit())
		{
			CloseEmulator();
		}
	} // while

} // RunEmulator()

// The Conntendo ToolBar
void toolStrip_ButtonClick(Object^ sender,ToolStripItemClickedEventArgs^ e)
{
	// Evaluate the Button property to determine which toolbar button was clicked
	Dev::DisableBatchTest();
	switch ( toolStrip->Items->IndexOf(e->ClickedItem) )
	{
	case MENU_BUTTON::Open:
		OpenROM();
		break;
	case MENU_BUTTON::Save:
		if (Emulator::IsLoaded())
		{
			Emulator::Save();
		}
		break;
	case MENU_BUTTON::Load:
		if (Emulator::IsLoaded())
		{
			Emulator::Load();
		}
		break;
	case MENU_BUTTON::Input:
		SpawnOptionsWindow();
		break;
	case MENU_BUTTON::Test:
		Emulator::RunGame( Dev::GetTestPath("castlevania3").c_str() );
		break;
	case MENU_BUTTON::Test2:
		Emulator::RunGame( Dev::GetTestPath("dragonwarrior").c_str() );
		break;
	case MENU_BUTTON::Batch:
		Dev::EnableBatchTest(false);
		break;

	} // switch

} // toolStrip_ButtonClick()

// General Options Menu with several Submenus
void menu_Options_ButtonClick(Object^ sender, ToolStripItemClickedEventArgs^ e)
{
	switch ( menu_Options->DropDownItems->IndexOf(e->ClickedItem) )
	{
	case MENU_OPTIONS::DisplayFPS:
		e->ClickedItem->BackColor = Dev::ToggleDisplayFPS() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_OPTIONS::NametableViewer:
		if (ntScreen.window == nullptr)
		{
			e->ClickedItem->BackColor = COLOR_ACTIVE;
			PPU::ToggleNametableViewer(true);
			ntScreen.Spawn();
		}
		else
		{
			e->ClickedItem->BackColor = COLOR_IDLE;
			PPU::ToggleNametableViewer(false);
			ntScreen.Destroy();
		}
		break;
	case MENU_OPTIONS::PatternTable:
		if (ptScreen.window == nullptr)
		{
			e->ClickedItem->BackColor = COLOR_ACTIVE;
			PPU::TogglePatternTableViewer(true);
			ptScreen.Spawn();
		}
		else
		{
			e->ClickedItem->BackColor = COLOR_IDLE;
			PPU::TogglePatternTableViewer(false);
			ptScreen.Destroy();
		}
		break;
	case MENU_OPTIONS::DrawScanlines:
		e->ClickedItem->BackColor = Emulator::ToggleDrawScanlines() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_OPTIONS::AllowNonIntegerScale:
		e->ClickedItem->BackColor = Emulator::ToggleAllowNonIntegerScaling() ? COLOR_ACTIVE : COLOR_IDLE;
		AdjustGameWindow();
		break;
	case MENU_OPTIONS::ScreenFilter:
		e->ClickedItem->BackColor = SetScreenFilter() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	}

} // menu_Options_ButtonClick()

// Audio Features Menu
void menu_Audio_ButtonClick(Object^ sender, ToolStripItemClickedEventArgs^ e)
{
	switch (menu_Audio->DropDownItems->IndexOf(e->ClickedItem))
	{
	case MENU_AUDIO::Increase:
		Emulator::AdjustVolume(true);
		break;
	case MENU_AUDIO::Decrease:
		Emulator::AdjustVolume(false);
		break;
	case MENU_AUDIO::Mute:
		e->ClickedItem->BackColor = APU::ToggleMuteAudio() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_AUDIO::DisablePulseA:
		e->ClickedItem->BackColor = APU::ToggleOneChannel(0) ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_AUDIO::DisablePulseB:
		e->ClickedItem->BackColor = APU::ToggleOneChannel(1) ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_AUDIO::DisableTriangle:
		e->ClickedItem->BackColor = APU::ToggleOneChannel(2) ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_AUDIO::DisableNoise:
		e->ClickedItem->BackColor = APU::ToggleOneChannel(3) ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_AUDIO::DisableSample:
		e->ClickedItem->BackColor = APU::ToggleOneChannel(4) ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	}

} // menu_Audio_ButtonClick()

// Running NES Menu
void menu_NES_ButtonClick(Object^ sender, ToolStripItemClickedEventArgs^ e)
{
	switch (menu_NES->DropDownItems->IndexOf(e->ClickedItem))
	{
	case MENU_NES::Reset:
		if (Emulator::IsLoaded())
		{
			Emulator::ShowMessage("RESET");
			Emulator::Reset();
		}
		break;
	case MENU_NES::Pause:
		if (Emulator::IsLoaded())
		{
			Emulator::Pause();
		}
		break;
	case MENU_NES::Eject:
		Emulator::Eject();
		break;
	case MENU_NES::Exit:
		CloseEmulator();
		break;
	}
	subMenu_DebugSpeedPrev = e->ClickedItem;

} // menu_NES_ButtonClick()

// Various Debug Sprite/Background Options Submenu
void subMenu_DebugDisplay_ButtonClick(Object^ sender, ToolStripItemClickedEventArgs^ e)
{
	e->ClickedItem->BackColor = COLOR_ACTIVE;

	switch (subMenu_DebugDisplay->DropDownItems->IndexOf(e->ClickedItem))
	{
	case MENU_DEBUGDISPLAY::HighlightSprites:
		e->ClickedItem->BackColor = PPU::ToggleHighlightSprites() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_DEBUGDISPLAY::DisableBackground:
		e->ClickedItem->BackColor = PPU::ToggleDisableBackground() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_DEBUGDISPLAY::DisableSpriteOffset:
		e->ClickedItem->BackColor = PPU::ToggleDisableSpriteOffset() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	case MENU_DEBUGDISPLAY::DisableSpriteAlpha:
		e->ClickedItem->BackColor = PPU::ToggleFillSprites() ? COLOR_ACTIVE : COLOR_IDLE;
		break;
	} // switch

} // subMenu_DebugDisplay_ButtonClick()

// Adjusting Emulator Speed Options Submenu
void subMenu_DebugSpeed_ButtonClick(Object^ sender, ToolStripItemClickedEventArgs^ e)
{
	e->ClickedItem->BackColor = COLOR_ACTIVE;
	if (subMenu_DebugSpeedPrev.operator->() != nullptr)
	{
		subMenu_DebugSpeedPrev->BackColor = COLOR_IDLE;
	}

	switch ( subMenu_DebugSpeed->DropDownItems->IndexOf(e->ClickedItem) )
	{
	case MENU_SPEED::Quarter:
		CPU::AdjustSpeed(0.25f);
		break;
	case MENU_SPEED::Half:
		CPU::AdjustSpeed(0.5f);
		break;
	case MENU_SPEED::Normal:
		CPU::AdjustSpeed(1.0f);
		break;
	case MENU_SPEED::TimeNHalf:
		CPU::AdjustSpeed(1.5f);
		break;
	case MENU_SPEED::Double:
		CPU::AdjustSpeed(2.0f);
		break;
	}
	subMenu_DebugSpeedPrev = e->ClickedItem;

} // subMenu_DebugSpeed_ButtonClick()

// Palette Selecting Options Submenu
void subMenu_Palette_ButtonClick(Object^ sender, ToolStripItemClickedEventArgs^ e)
{
	e->ClickedItem->BackColor = COLOR_ACTIVE;
	if (subMenu_PalettePrev.operator->() != nullptr)
	{
		subMenu_PalettePrev->BackColor = COLOR_IDLE;
	}

	switch (subMenu_Palette->DropDownItems->IndexOf(e->ClickedItem))
	{
	case Palette::Colorspace::NES_COMP:
		Palette::SetPalette(Palette::Colorspace::NES_COMP);
		break;
	case Palette::Colorspace::NES_RGB:
		Palette::SetPalette(Palette::Colorspace::NES_RGB);
		break;
	case Palette::Colorspace::GAMEBOY:
		Palette::SetPalette(Palette::Colorspace::GAMEBOY);
		break;
	case Palette::Colorspace::VIRTUALBOY:
		Palette::SetPalette(Palette::Colorspace::VIRTUALBOY);
		break;
	case Palette::Colorspace::COMMODORE64:
		Palette::SetPalette(Palette::Colorspace::COMMODORE64);
		break;
	case Palette::Colorspace::CGA:
		Palette::SetPalette(Palette::Colorspace::CGA);
		break;
	case Palette::Colorspace::EGA:
		Palette::SetPalette(Palette::Colorspace::EGA);
		break;
	} // switch
	subMenu_PalettePrev = e->ClickedItem;

} // subMenu_Palette_ButtonClick()

void menu_ClosedClick( Object^ sender, FormClosingEventArgs^ e)
{
	CloseEmulator();

} // menu_ClosedClick()

void menu_OnLoad(Object^ sender, EventArgs^ e)
{
	Emulator::Setup(mainScreen.renderer);
	DetectMonitors(); // Get Display Info
	Joypad::SetupConfig();
	RunEmulator();

} // menu_OnLoad()

// Adjusts pos and size of Game Window to fit inside Menu Window
void AdjustGameWindow()
{
	float newScale = (menuForm->Height - TOOLBAR_HEIGHT * 2) / (float)HEIGHT;
	if ( !Emulator::IsNonIntegerScalingAllowed() )
	{
		newScale = (int)newScale;
	}
	int newPosX  = ( menuForm->Width - (WIDTH*newScale) - PADDING ) / 2.0f;
	int newPosY  = ( menuForm->Height - (HEIGHT*newScale) ) / 2.0f;
	SetWindowPos(winHandle, nullptr, newPosX, newPosY, WIDTH*newScale, HEIGHT*newScale, SWP_SHOWWINDOW);

} // AdjustGameWindow()

void menu_Resized( Object^ sender, EventArgs^ e )
{
	AdjustGameWindow();

} // menu_Resized()

void SpawnMainWindow()
{
	// Init SDL
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, UNFILTERED); // Set Screen Filtering

	// Setup Game Window and Renderer (Hide until embeded to WinForm Window)
	mainScreen.window = SDL_CreateWindow(mainScreen.name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH*SCALE, HEIGHT*SCALE, SDL_WINDOW_HIDDEN);
	mainScreen.renderer = SDL_CreateRenderer(mainScreen.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetLogicalSize(mainScreen.renderer, WIDTH, HEIGHT);
	SDL_SetRenderDrawBlendMode(mainScreen.renderer, SDL_BLENDMODE_BLEND);
	mainScreen.windowID = SDL_GetWindowID(mainScreen.window);

	// Embed SDL2 Game Window inside Menu Window 
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version); // This line fixed Release Build Issue! SDL Wiki says that info.version must be initialised before usage  
	SDL_GetWindowWMInfo(mainScreen.window, &info);
	winHandle = info.info.win.window;
	SetParent(winHandle, connHandle);
	SDL_ShowWindow(mainScreen.window);

	// SDL2 Video Fixed Settings
	Color backDrop = COLOR_BACKDROP;
	SDL_SetWindowBordered(mainScreen.window, SDL_FALSE);
	SDL_SetWindowResizable(mainScreen.window, SDL_FALSE);
	SDL_SetRenderDrawColor(mainScreen.renderer, backDrop.R, backDrop.G, backDrop.B, backDrop.A);

	// Place Emulator Window inside Menu Window
	AdjustGameWindow();

} // SpawnMainWindow()

// Initialize WinForm Window and Menus
void SetupWinForms()
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	// Setup ToolBar
	toolStripContainer = gcnew ToolStripContainer();
	toolStrip = gcnew ToolStrip();
	toolStripContainer->TopToolStripPanel->Controls->Add(toolStrip);
	toolStrip->BackColor = COLOR_TOOLBAR;
	toolStrip->AutoSize = false;
	toolStrip->Height = TOOLBAR_HEIGHT;

	// MENU: NES
	menu_NES = gcnew ToolStripMenuItem();
	menu_NES->Text = "NES";
	menu_NES->DropDownItems->Add("Reset        F2");
	menu_NES->DropDownItems->Add("Pause        P");
	menu_NES->DropDownItems->Add("Eject        Esc");
	menu_NES->DropDownItems->Add("Exit          Esc");
	menu_NES->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&menu_NES_ButtonClick);

	// SUBMENU: DebugSpeed 
	subMenu_DebugSpeed = gcnew ToolStripMenuItem();
	subMenu_DebugSpeed->Text = "Emulator Speed";
	subMenu_DebugSpeed->DropDownItems->Add("0.25x");
	subMenu_DebugSpeed->DropDownItems->Add("0.5x");
	subMenu_DebugSpeed->DropDownItems->Add("1.0x");
	subMenu_DebugSpeed->DropDownItems->Add("1.5x");
	subMenu_DebugSpeed->DropDownItems->Add("2.0x");
	subMenu_DebugSpeed->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&subMenu_DebugSpeed_ButtonClick);

	// SUBMENU: Palette 
	subMenu_Palette = gcnew ToolStripMenuItem();
	subMenu_Palette->Text = "Color Palette";
	subMenu_Palette->DropDownItems->Add("NES Composite");
	subMenu_Palette->DropDownItems->Add("NES RGB");
	subMenu_Palette->DropDownItems->Add("Game Boy");
	subMenu_Palette->DropDownItems->Add("Virtual Boy");
	subMenu_Palette->DropDownItems->Add("Commodore 64");
	subMenu_Palette->DropDownItems->Add("CGA DOS");
	subMenu_Palette->DropDownItems->Add("EGA DOS");
	subMenu_Palette->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&subMenu_Palette_ButtonClick);

	// SUBMENU: DebugDisplay Options
	subMenu_DebugDisplay = gcnew ToolStripMenuItem();
	subMenu_DebugDisplay->Text = "Debug Display";
	subMenu_DebugDisplay->DropDownItems->Add("Highlight Sprites");
	subMenu_DebugDisplay->DropDownItems->Add("Disable Background");
	subMenu_DebugDisplay->DropDownItems->Add("Disable Sprite Offset");
	subMenu_DebugDisplay->DropDownItems->Add("Disable Sprite Alpha");
	subMenu_DebugDisplay->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&subMenu_DebugDisplay_ButtonClick);

	// MENU: Options Menu
	menu_Options = gcnew ToolStripMenuItem();
	menu_Options->Text = "OPTIONS";
	menu_Options->DropDownItems->Add("Display FPS");
	menu_Options->DropDownItems->Add("Nametable Viewer");
	menu_Options->DropDownItems->Add("PatternTable Viewer");
	menu_Options->DropDownItems->Add("Draw Scanlines");
	menu_Options->DropDownItems->Add("Allow Non-Integer Scaling");
	//menu_Options->DropDownItems->Add("Filter Screen"); // Currently Disabled
	menu_Options->DropDownItems->Add(subMenu_DebugDisplay);
	menu_Options->DropDownItems->Add(subMenu_DebugSpeed);
	menu_Options->DropDownItems->Add(subMenu_Palette);
	menu_Options->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&menu_Options_ButtonClick);

	// MENU: Audio
	menu_Audio = gcnew ToolStripMenuItem();
	menu_Audio->Text = "AUDIO";
	menu_Audio->DropDownItems->Add("+Vol        +");
	menu_Audio->DropDownItems->Add("-Vol         -");
	menu_Audio->DropDownItems->Add("Mute All Sound");
	menu_Audio->DropDownItems->Add("Disable PulseA");
	menu_Audio->DropDownItems->Add("Disable PulseB");
	menu_Audio->DropDownItems->Add("Disable Triangle");
	menu_Audio->DropDownItems->Add("Disable Noise");
	menu_Audio->DropDownItems->Add("Disable Sample");
	menu_Audio->DropDownItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&menu_Audio_ButtonClick);

	// Populate Toolbar Buttons
	toolStrip->Items->Add(menu_NES);
	toolStrip->Items->Add("OPEN");
	toolStrip->Items->Add("SAVE");
	toolStrip->Items->Add("LOAD");
	toolStrip->Items->Add(menu_Options);
	toolStrip->Items->Add("INPUT");
	toolStrip->Items->Add(menu_Audio);

#if DEV_BUILD
	// Dev Toolbar Buttons
	toolStrip->Items->Add("TEST");
	toolStrip->Items->Add("TEST2");
	toolStrip->Items->Add("BATCH");
#endif

	// Setup Menu Window
	menuForm = gcnew Conntendo::ConnForm;
	menuForm->Width = WIDTH * SCALE + PADDING;
	menuForm->Height = (HEIGHT * SCALE) + TOOLBAR_HEIGHT * 2;
	menuForm->BackColor = COLOR_BACKDROP;

	// Set Title
	String^ titleString = gcnew String(Emulator::GetEmulatorTitle().c_str());
	menuForm->Text = titleString;

	// Setup Toolbar
	toolStrip->ItemClicked += gcnew System::Windows::Forms::ToolStripItemClickedEventHandler(&toolStrip_ButtonClick);
	menuForm->Controls->Add(toolStrip);

	// Bind to Menu Window Events
	menuForm->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(&menu_ClosedClick);
	menuForm->Shown += gcnew System::EventHandler(&menu_OnLoad);
	menuForm->Resize += gcnew System::EventHandler(&menu_Resized);

	// Get ref to WinForm Window
	connHandle = static_cast<HWND>(menuForm->Handle.ToPointer());

} // SetupWinForms()

// Entry Point
int main(int argc, char *argv[])
{
	// Setup SDL and WinForms
	SetupWinForms();
	SpawnMainWindow();

	// Run Emulator from WinForm
	Application::Run(menuForm);

	// Closing Conntendo
	return 0;

} // main()