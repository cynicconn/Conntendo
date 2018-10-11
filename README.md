# Conntendo
x86 NES Emulator written in C++ using WinForms and SDL2 API. I've favored clarity and readability instead of compressed, optimized code. The Emulator has over 90% compatibility with the entire NES Library.

## Screenshots
<p float="center">
  <img src="https://user-images.githubusercontent.com/27335324/46614594-d7161f80-cadb-11e8-90ed-ff381d46e832.png" width="260" />
  <img src="https://user-images.githubusercontent.com/27335324/46614548-ba79e780-cadb-11e8-83a8-defd2ed7eada.png" width="260" /> 
  <img src="https://user-images.githubusercontent.com/27335324/46614937-a682b580-cadc-11e8-8eb5-d22d3371e208.png" width="260" />
</p>
<p float="center">
  <img src="https://user-images.githubusercontent.com/27335324/46642102-e4162b80-cb3a-11e8-9302-91c8cc206d41.png" width="260" />
  <img src="https://user-images.githubusercontent.com/27335324/46642137-145dca00-cb3b-11e8-9d83-fc50db7ae128.png" width="260" /> 
  <img src="https://user-images.githubusercontent.com/27335324/46642154-2a6b8a80-cb3b-11e8-8dda-91b9d0463418.png" width="260" />
</p>

## Building and Running
The Conntendo project was built as an x86 Wndows Application. Other Builds have not been tested.

The project was created with the following;
* Visual Studios 2017
* C++/CLR
* .NET Framework 4.7
* SDL2
* zlib
* Blargg Audio Library

This project includes the VS files along with all requird library and header files for SDL2, zlib, and the Third Party Audio Library. The project is Relative-Path safe and should run from any storage location.
The contents of Folder _BuildDependancies_ must be put into the Debug/Release folder of the compiled Conntendo project.
The executable will not run unless all Resources and library files are found.

## Features
Conntendo has pretty standard emulator features; savestates, input customization, joystick support, etc.
There are several PPU options including;
* Nametable Viewer
* Patterntable Viewer
* Color Palette Swap (to mimic other console palettes)
* Toggle Sprite/Background Rendering

## Mappers
Conntendo has the following Mappers implemented; giving the emulator over 90% compatibility

* Mapper 0: 	Stock NES
* Mapper 1: 	MMC1
* Mapper 2: 	UxROMs
* Mapper 3: 	CNROMs
* Mapper 4: 	MMC3
* Mapper 5: 	MMC5
* Mapper 7: 	AxROMs
* Mapper 9: 	MMC2
* Mapper 10:	MMC4
* Mapper 11:	Color Dreams
* Mapper 25:	VRC4^
* Mapper 66: 	GxROMs
* Mapper 69: 	Sunsoft FME-7

^ Denotes partial functionality

## Help
The following is a list of resources I used to make this emulation project possible;
* __Shay Green's Audio Libraries__: http://blargg.8bitalley.com/libs/audio.html#Nes_Snd_Emu
* __Nesdev Wiki__: http://wiki.nesdev.com/w/index.php/Nesdev_Wiki
* __6502 OpCodes__: http://www.6502.org/tutorials/6502opcodes.html

## Project History
Date | Milestone
------------- | -------------
Aug 2017  | Project Started
Mar 2018  | Version 1.0 Released
Sep 2018  | Version 2.0 Released
Oct 2018  | Source Code Released
