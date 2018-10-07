#include "joypad.h"

// Conntendo
#include "emulator.h"
#include "ppu.h"
#include "cartridge.h"
#include "files.h"

// Config
#define INPUT_CONFIG		"Input"

// Keyboard Shortcuts
#define SHORTCUT_ESCAPE		SDL_SCANCODE_ESCAPE
#define SHORTCUT_PAUSE		SDL_SCANCODE_P
#define SHORTCUT_RESET		SDL_SCANCODE_F2
#define SHORTCUT_QUICKSAVE	SDL_SCANCODE_F5
#define SHORTCUT_QUICKLOAD	SDL_SCANCODE_F9
#define SHORTCUT_VERINFO	SDL_SCANCODE_F11
#define SHORTCUT_VOL_UP		SDL_SCANCODE_KP_PLUS
#define SHORTCUT_VOL_DOWN	SDL_SCANCODE_KP_MINUS

#if DEV_BUILD
#define SHORTCUT_DEBUG_INCR	SDL_SCANCODE_N
#define SHORTCUT_DEBUG_DECR	SDL_SCANCODE_M
#endif

namespace Joypad
{
	const int DEAD_ZONE = 13500;

	// Input Config 
	InputConfig keyboard[2];
	InputConfig joypad[2];

	// Recording Input
	AssignInput assignInput;

	u8 joypad_bits[2];  // Joypad shift registers
	bool strobe;		// Joypad strobe latch
	SDL_Joystick* joystick[] = { nullptr,nullptr };

	u8 input[2] = { 0,0 };
	bool buttonsPressed[256];

	// Display Names for each Button
	string buttonNames[BUTTON_COUNT] =
	{
		"A",
		"B",
		"SELECT",
		"START",
		"UP",
		"DOWN",
		"LEFT",
		"RIGHT"
	};

	void SaveConfig()
	{
		string configPath = Emulator::GetConfigPath() + INPUT_CONFIG + CONFIG_EXT;
		Files::SaveConnFile(configPath, Files::FileType::Joypad);

	} // SaveInputConfig()

	// Set all Input types to Default Layout
	void DefaultAll()
	{
		keyboard[0].DefaultKeyboard_P1();
		keyboard[1].DefaultKeyboard_P2();
		joypad[0].DefaultJoypad();
		joypad[1].DefaultJoypad();

	} // DefaultAll()

	void SetupConfig()
	{
		DefaultAll();
		string configPath = Emulator::GetConfigPath() + INPUT_CONFIG + CONFIG_EXT;
		Files::LoadConnFile(configPath, Files::FileType::Joypad);

	} // SetupInputConfig()

	void ToggleStrobe(bool isHigh )
	{
		// Read the joypad data on strobe's transition 1 -> 0.
		if ( strobe &&  !isHigh)
		{
			for (int i = 0; i < 2; i++)
			{
				joypad_bits[i] = input[i];
			} // for
		}
		strobe = isHigh;

	} // ToggleStrobe()

	void SetJoystick( int index, SDL_Joystick* joy)
	{
		joystick[index] = joy;

	} // SetJoystick()

	void InitJoysticks()
	{
		for (int i = 0; i < SDL_NumJoysticks(); i++)
		{
			SetJoystick( i, SDL_JoystickOpen(i) );
		} // for

	} // InitJoysticks()

	// Read joypad state (NES register format)
	u8 GetInput(int plyr)
	{
		// When strobe is high, it keeps reading A:
		if (strobe)
		{
			return 0x40 | ( input[0] & 0x01 );
		}

		// Get the status of a button and shift the register:
		u8 joy				= 0x40 | ( joypad_bits[plyr] & 0x01 );
		joypad_bits[plyr]	= 0x80 | ( joypad_bits[plyr] >> 1 );
		return joy;

	} // GetInput()

	// Check if button is Pressed ( and previously wasnt )
	bool CheckButton( const u8* state, int index )
	{
		if ( state[index] && !buttonsPressed[index] )
		{
			buttonsPressed[index] = true;
			return true;
		}
		else if (!state[index])
		{
			buttonsPressed[index] = false;
		}
		return false;

	} // CheckButton()

	u8 GetJoystickInput( Joypad::INPUT inputIndex, u8 player )
	{
		InputConfig* inputConfig = &joypad[player];

		bool testNeg = false;
		int getAxis = 0;

		// Get Input based on Button Type
		switch (inputConfig->bindType[inputIndex])
		{
		case SDL_CONTROLLER_BINDTYPE_AXIS:
			testNeg = (inputConfig->axisDir[inputIndex] < 0);
			getAxis = SDL_JoystickGetAxis(joystick[player], inputConfig->buttonCodes[inputIndex]);
			if (testNeg && getAxis < -DEAD_ZONE || !testNeg && getAxis > DEAD_ZONE)
			{
				return true;
			}
			break;
		case SDL_CONTROLLER_BINDTYPE_HAT:
			return ( SDL_JoystickGetHat(joystick[player], 0) == inputConfig->buttonCodes[inputIndex] );
			break;
		case SDL_CONTROLLER_BINDTYPE_BUTTON:
			return SDL_JoystickGetButton(joystick[player], inputConfig->buttonCodes[inputIndex]);
			break;
		} // switch 
		return false;

	}// GetJoystickInput()

	void SetRecordMode( RecordMode newMode, int playerIndex )
	{
		assignInput.recordMode		= newMode;
		assignInput.currentInput	= 0;
		assignInput.currentPlayer	= playerIndex;
		switch( newMode )
		{
		case RecordMode::Keyboard:
			keyboard[playerIndex].Clear();
			break;
		case RecordMode::Joypad:
			joypad[playerIndex].Clear();
			break;
		case RecordMode::None:
			break;

		} // switch

	} // SetRecordMode()

	bool IsRecordingInput()
	{
		return (assignInput.recordMode != RecordMode::None &&  assignInput.currentInput < BUTTON_COUNT );

	} // IsRecordingInput()

	string GetCurrentRecording()
	{
		return "Button: " + buttonNames[assignInput.currentInput];

	} // GetCurrentRecording()

	void AssignPlayerInput(const u8* state)
	{
		int plyr = assignInput.currentPlayer;
		if (assignInput.recordMode == RecordMode::Keyboard)
		{
			for (int i = 0; i < 255; i++)
			{
				if (state[i] && !keyboard[plyr].IsKeyTaken(i) )
				{
					keyboard[plyr].tempCodes[assignInput.currentInput] = i;
					assignInput.currentInput++;
					break;
				}
			} // for

			  // Finished
			if (assignInput.currentInput >= BUTTON_COUNT)
			{
				keyboard[plyr].ApplyNewInput();
				Joypad::SaveConfig();
			}
		}
		else if (assignInput.recordMode == RecordMode::Joypad)
		{
			// Axis
			for (int i = 0; i < 6; i++)
			{
				int getAxis = SDL_JoystickGetAxis(joystick[plyr], i);
				if (getAxis >= INT16_MAX || getAxis <= INT16_MIN)
				{
					getAxis = 0;
				}
				int axisDir = (SDL_JoystickGetAxis(joystick[plyr], i) < 0) ? -1 : 1;
				if ( abs(getAxis) > DEAD_ZONE && !joypad[plyr].IsAxisTaken(i, axisDir) )
				{
					joypad[plyr].tempCodes[assignInput.currentInput]	= i;
					joypad[plyr].tempDir[assignInput.currentInput]		= axisDir; // Negative or Positive
					joypad[plyr].tempType[assignInput.currentInput]	= SDL_CONTROLLER_BINDTYPE_AXIS;
					assignInput.currentInput++;
					break;
				}
			} // for

			// Buttons
			for (int i = 0; i < 15; i++)
			{
				if (SDL_JoystickGetButton(joystick[plyr], i) && !joypad[plyr].IsButtonTaken(i, SDL_CONTROLLER_BINDTYPE_BUTTON) )
				{
					joypad[plyr].tempCodes[assignInput.currentInput]	= i;
					joypad[plyr].tempType[assignInput.currentInput]		= SDL_CONTROLLER_BINDTYPE_BUTTON;
					assignInput.currentInput++;
					break;
				}
			} // for

			// HAT
			int hatVal = SDL_JoystickGetHat( joystick[plyr], 0);
			if (hatVal != SDL_HAT_CENTERED && !joypad[plyr].IsButtonTaken(hatVal, SDL_CONTROLLER_BINDTYPE_HAT) )
			{
				joypad[plyr].tempCodes[assignInput.currentInput]	= hatVal;
				joypad[plyr].tempType[assignInput.currentInput]	= SDL_CONTROLLER_BINDTYPE_HAT;
				assignInput.currentInput++;
			}

			  // Finished
			if (assignInput.currentInput >= BUTTON_COUNT)
			{
				joypad[plyr].ApplyNewInput();
				Joypad::SaveConfig();
			}

		} // elseif

	} // AssignPlayerInput()

	// Take current Keyboard/Josytick State and apply it as NES Controller input, also Emulator Controls
	u8 ProcessPlayerInput(const u8* state)
	{
		// Zero out NES Input
		for (int i = 0; i < 2; i++)
		{
			input[i] = 0;
		} // for

		// Keyboard
		for (int i = 0; i < 2; i++)
		{
			int* buttons = keyboard[i].buttonCodes;

			input[i] |= (state[ buttons[INPUT::iA]		])	* BUTTON::bA;
			input[i] |= (state[ buttons[INPUT::iB]		])	* BUTTON::bB;
			input[i] |= (state[ buttons[INPUT::iSELECT]	])	* BUTTON::bSELECT;
			input[i] |= (state[ buttons[INPUT::iSTART]	])	* BUTTON::bSTART;
			input[i] |= (state[ buttons[INPUT::iUP]		])	* BUTTON::bUP;
			input[i] |= (state[ buttons[INPUT::iDOWN]	])	* BUTTON::bDOWN;
			input[i] |= (state[ buttons[INPUT::iLEFT]	])	* BUTTON::bLEFT;
			input[i] |= (state[ buttons[INPUT::iRIGHT]	])	* BUTTON::bRIGHT;
		} // for

		// Joypads
		for (int i = 0; i < SDL_NumJoysticks(); i++)
		{		
			input[i] |= GetJoystickInput( INPUT::iA,	  i)  * BUTTON::bA;
			input[i] |= GetJoystickInput( INPUT::iB,	  i)  * BUTTON::bB;
			input[i] |= GetJoystickInput( INPUT::iSELECT, i)  * BUTTON::bSELECT;
			input[i] |= GetJoystickInput( INPUT::iSTART,  i)  * BUTTON::bSTART;
												 						
			input[i] |= GetJoystickInput( INPUT::iUP,     i)  * BUTTON::bUP;
			input[i] |= GetJoystickInput( INPUT::iDOWN,   i)  * BUTTON::bDOWN;
			input[i] |= GetJoystickInput( INPUT::iLEFT,   i)  * BUTTON::bLEFT;
			input[i] |= GetJoystickInput( INPUT::iRIGHT,  i)  * BUTTON::bRIGHT;

		} // for

		ProcessEmulatorInput(state);

		return 0;

	} // ProcessPlayerInput()

	void ProcessEmulatorInput(const u8* state)
	{
		// Emulator Controls
		if (CheckButton(state, SHORTCUT_ESCAPE))
		{
			if (Emulator::IsLoaded())
			{
				Emulator::Eject();
			}
			else
			{
				Emulator::ShutDown();
			}
		}
		if (CheckButton(state, SHORTCUT_PAUSE))
		{
			if (Emulator::IsLoaded())
			{
				Emulator::Pause();
			}
		}
		else if (CheckButton(state, SHORTCUT_RESET))
		{
			if (Emulator::IsLoaded())
			{
				Emulator::ShowMessage("RESET");
				Emulator::Reset();
			}
		}
		else if (CheckButton(state, SHORTCUT_QUICKSAVE))
		{
			if (Emulator::IsLoaded())
			{
				Emulator::Save();
			}
		}
		else if (CheckButton(state, SHORTCUT_QUICKLOAD))
		{
			if (Emulator::IsLoaded())
			{
				Emulator::Load();
			}
		}
		else if (CheckButton(state, SHORTCUT_VERINFO))
		{
			string verMessage = "Emulator Ver: " + Emulator::GetVersionNumber();
			Emulator::ShowMessage(verMessage);
		}
		else if (CheckButton(state, SHORTCUT_VOL_UP))
		{
			Emulator::AdjustVolume(true);
		}
		else if (CheckButton(state, SHORTCUT_VOL_DOWN))
		{
			Emulator::AdjustVolume(false);
		}
#if DEV_BUILD
		else if (CheckButton(state, SHORTCUT_DEBUG_INCR))
		{
			PPU::IncrementDebugValue(true);
		}
		else if (CheckButton(state, SHORTCUT_DEBUG_DECR))
		{
			PPU::IncrementDebugValue(false);
		}
#endif


	} // ProcessEmulatorInput()

	void GetSaveInputConfig( InputConfig* savedInput )
	{
		savedInput[0] = keyboard[0];
		savedInput[1] = keyboard[1];
		savedInput[2] = joypad[0];
		savedInput[3] = joypad[1];

	} // SaveInputConfig()

	void LoadInputConfig(InputConfig loadedConfig[] )
	{
		//Emulator::ShowMessage("Loaded Input");
		keyboard[0]		= loadedConfig[0];
		keyboard[1]		= loadedConfig[1];
		joypad[0]		= loadedConfig[2];
		joypad[1]		= loadedConfig[3];

	} // LoadInputConfig()

} // Joypad