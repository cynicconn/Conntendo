#pragma once
//----------------------------------------------------------------//
// For capturing and outputting Keyboard and Joypad Input
// Customizing Input is handled here too
// Currently supports Two Controllers; Keyboard and Joystick for both
//----------------------------------------------------------------//

#include "common.h"

// SDL
#include "SDL_scancode.h"
#include "SDL_gamecontroller.h"

namespace Joypad
{
	const int BUTTON_COUNT = 8;

	enum RecordMode
	{
		None,
		Keyboard,
		Joypad
	}; // RecordMode

	// Input Code for Emulator
	enum INPUT
	{
		iA		= 0,
		iB		= 1,
		iSELECT = 2,
		iSTART	= 3,
		iUP		= 4,
		iDOWN	= 5,
		iLEFT	= 6,
		iRIGHT	= 7

	}; // BUTTON

	// Coresponds to NES
	enum BUTTON
	{
		bA		= 0x01,
		bB		= 0x02,
		bSELECT = 0x04,
		bSTART	= 0x08,
		bUP		= 0x10,
		bDOWN	= 0x20,
		bLEFT	= 0x40,
		bRIGHT	= 0x80

	}; // BUTTON

	struct AssignInput
	{
		RecordMode recordMode; 
		int currentInput; 
		int currentPlayer;

		AssignInput() 
		{
			recordMode		= RecordMode::None;
			currentInput	= 0;
			currentPlayer	= 0;
		}

	}; // AssignInput

	// Struct for storing Inputs
	struct InputConfig
	{
		int buttonCodes[BUTTON_COUNT];
		int bindType[BUTTON_COUNT]; // For Joypad Only, 0 = null
		int axisDir[BUTTON_COUNT]; // For Joypad Only 0 = null, -1 = neg, 1 = pos

		// for caching new Inputs until all are selected 
		int tempCodes[BUTTON_COUNT];
		int tempType[BUTTON_COUNT];
		int tempDir[BUTTON_COUNT];

		InputConfig() 
		{
			int allocate = sizeof(buttonCodes);
			memset(buttonCodes, 0, allocate);
			memset(bindType, 0, allocate);
			memset(axisDir, 0, allocate);
			memset(tempCodes, -1, allocate);
			memset(tempType, 0, allocate);
			memset(tempDir, 0, allocate);
		};

		void Clear()
		{
			for (int i = 0; i < BUTTON_COUNT; i++)
			{
				tempCodes[i]	= -1;
				axisDir[i]		=  0;
				bindType[i]		=  0;
			}
		} // Clear()

		void DefaultKeyboard_P1()
		{ 
			buttonCodes[INPUT::iA]		= SDL_SCANCODE_Z;
			buttonCodes[INPUT::iB]		= SDL_SCANCODE_X;
			buttonCodes[INPUT::iSELECT]	= SDL_SCANCODE_A;
			buttonCodes[INPUT::iSTART]	= SDL_SCANCODE_S;
			buttonCodes[INPUT::iUP]		= SDL_SCANCODE_UP;
			buttonCodes[INPUT::iDOWN]	= SDL_SCANCODE_DOWN;
			buttonCodes[INPUT::iLEFT]	= SDL_SCANCODE_LEFT;
			buttonCodes[INPUT::iRIGHT]	= SDL_SCANCODE_RIGHT;
		}

		void DefaultKeyboard_P2()
		{
			buttonCodes[INPUT::iA]		= SDL_SCANCODE_KP_4;
			buttonCodes[INPUT::iB]		= SDL_SCANCODE_KP_6;
			buttonCodes[INPUT::iSELECT] = SDL_SCANCODE_KP_0;
			buttonCodes[INPUT::iSTART]	= SDL_SCANCODE_KP_ENTER;
			buttonCodes[INPUT::iUP]		= SDL_SCANCODE_KP_5;
			buttonCodes[INPUT::iDOWN]	= SDL_SCANCODE_KP_2;
			buttonCodes[INPUT::iLEFT]	= SDL_SCANCODE_KP_1;
			buttonCodes[INPUT::iRIGHT]	= SDL_SCANCODE_KP_3;
		}

		void DefaultJoypad()
		{
			buttonCodes[INPUT::iA]		= SDL_CONTROLLER_BUTTON_A;
			buttonCodes[INPUT::iB]		= SDL_CONTROLLER_BUTTON_B;
			buttonCodes[INPUT::iSELECT]	= 6;
			buttonCodes[INPUT::iSTART]	= 7;
			buttonCodes[INPUT::iUP]		= SDL_CONTROLLER_AXIS_LEFTY;
			buttonCodes[INPUT::iDOWN]	= SDL_CONTROLLER_AXIS_LEFTY;
			buttonCodes[INPUT::iLEFT]	= SDL_CONTROLLER_AXIS_LEFTX;
			buttonCodes[INPUT::iRIGHT]	= SDL_CONTROLLER_AXIS_LEFTX;
			for (int i = 0; i < BUTTON_COUNT; i++)
			{
				bindType[i] = (i <= 3) ? SDL_CONTROLLER_BINDTYPE_BUTTON : SDL_CONTROLLER_BINDTYPE_AXIS;
			} // for

			// Set Axis Direction
			axisDir[INPUT::iUP]		= -1;
			axisDir[INPUT::iDOWN]	=  1;
			axisDir[INPUT::iLEFT]	= -1;
			axisDir[INPUT::iRIGHT]	=  1;
		}

		void ApplyNewInput()
		{
			memcpy( buttonCodes, tempCodes, sizeof(tempCodes) );
			memcpy( axisDir,	 tempDir,	sizeof(tempDir) );
			memcpy( bindType,	 tempType,	sizeof(tempType) );

		} // ApplyNewButtons()

		bool IsKeyTaken(int index)
		{
			for (int i = 0; i < BUTTON_COUNT; i++)
			{
				if (tempCodes[i] == index)
				{
					return true;
				}
			} // for
			return false;

		} // IsKeyTaken()

		bool IsAxisTaken(int index, int dir )
		{
			for (int i = 0; i < BUTTON_COUNT; i++)
			{
				if (tempCodes[i] == index && tempDir[i] == dir )
				{
					return true;
				}
			} // for
			return false;

		} // IsAxisTaken()

		bool IsButtonTaken(int index, int targetType )
		{
			for (int i = 0; i < BUTTON_COUNT; i++)
			{
				if (tempCodes[i] == index && tempType[i] == targetType)
				{
					return true;
				}
			} // for
			return false;

		} // IsButtonTaken()

	};

	void GetSaveInputConfig(InputConfig* savedInput);
	void LoadInputConfig(InputConfig loadedConfig[] );

	// Input Config
	void SaveConfig();
	void SetupConfig();

	u8 GetInput(int n);
	void ToggleStrobe(bool v);

	bool IsRecordingInput();
	string GetCurrentRecording();
	void SetRecordMode(RecordMode newMode, int playerIndex);

	u8 ProcessPlayerInput(const u8* state);
	void ProcessEmulatorInput(const u8* state);
	void AssignPlayerInput(const u8* state);

	void InitJoysticks();

	void DefaultAll();

} // Joypad
