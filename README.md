# Conntendo
x86 NES Emulator written in C++ using WinForms and SDL API 

## Screenshots
<p float="left">
  <img src="https://user-images.githubusercontent.com/27335324/46614594-d7161f80-cadb-11e8-90ed-ff381d46e832.png" width="260" />
  <img src="https://user-images.githubusercontent.com/27335324/46614548-ba79e780-cadb-11e8-83a8-defd2ed7eada.png" width="260" /> 
  <img src="https://user-images.githubusercontent.com/27335324/46614937-a682b580-cadc-11e8-8eb5-d22d3371e208.png" width="260" />
</p>

## Building and Running
The project was created with the following;
* Visual Studios 2017
* C++/CLR
* .NET Framework 4.7
* SDL2
* zlib

This project includes the VS files along with the SDL and zlib DLLs. 
The contents of Folder _EmulatorDependancies_ must be put into the Debug/Release folder of the compiled Conntendo project.

## Mappers
Conntendo has the following Mappers implemented; giving the emulator over 90% compatibility

* Mapper 0: 	Stock NES
* Mapper 1: 	MMC1
* Mapper 2: 	UxROMs
* Mapper 3: 	CNROMs
* Mapper 4: 	MMC3
* Mapper 5: 	MMC5
* Mapper 7: 	AxROMs ( Rare Ltd Games )
* Mapper 9: 	MMC2 ( exclusively for Punch Out )
* Mapper 10:	MMC4
* Mapper 11:	Color Dreams
* Mapper 25^:	VRC4 
* Mapper 66: 	GxROMs
* Mapper 69: 	Sunsoft FME-7

^ Denotes partial functionality

## Help
The following is a list of resources I used to make this emulation project possible;
* __Shay Green's Audio Libraries__: http://blargg.8bitalley.com/libs/audio.html#Nes_Snd_Emu
* __Nesdev Wiki__: http://wiki.nesdev.com/w/index.php/Nesdev_Wiki

## Project History
Date | Milestone
------------- | -------------
Aug 2017  | Project Started
Mar 2018  | Version 1.0 Released
Sep 2018  | Version 2.0 Released
Oct 2018  | Source Code Uploaded
