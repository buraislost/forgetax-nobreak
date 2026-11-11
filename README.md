# ForgeTax No-Break

Made by bura.

A small single-player trainer for ForgeTax. While enabled it sets the break (destruction) chance to 0%.

## How it works
- `autoinject.exe` waits for `ForgeTax.exe`, loads `autobreak.dll` into it once per game launch, and draws a small click-through overlay on the game window.
- `autobreak.dll` finds the game's chance tables through the Mono runtime and keeps them zeroed. It restores the original values when switched off.
- **F8** toggles it in game. Current state is written to `status.txt` next to the DLL.

## Build
Open a terminal in this folder and run `build.bat` (Visual Studio with the C++ x64 tools is required). Output goes to `bin\`.

## Use
1. Run `bin\autoinject.exe` and leave it open.
2. Start the game. Use Windowed or Borderless mode so the overlay is visible.

## Notes
- For single-player use only. Do not use it in any online mode.
- It modifies the memory of another process, so antivirus or Smart App Control may flag or block it.
- Not affiliated with the game or its developers.
