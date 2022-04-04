# Small Met(ronome)
A small metronome app written in C++ and SDL. It features a singular dark theme, which may become customizable as more changes are made to it. Currently only for Windows, but I hope to add support for Mac and Linux later on.

## How to use?
The large button in the middle starts/stops playback. The slider in the middle (for the time being) can be used to change the BPM, by dragging it to the left or right. I'm keeping this program a console application (also for the time being), because I'm trying to figure out how to render text without adding more external files required by the program.

## Task list
- [x] Create the base of the UI (custom titlebar and buttons)
- [x] Add a simple audio part to the program
- [ ] Add a popup menu to the top left so I can add a customize screen & other functions
- [ ] Fix the audio timing errors (maybe create a separate thread for it?)
- [ ] Add text rendering so that I can remove the console

## Why?
I play guitar. This small project was simply the result of me wanting something small that would get the job of a metronome done, that wouldn't use too much memory or CPU on my computer. I also wanted to gain some experience and start off by creating something small from scratch for the first time.

## Dependiences
[SDL](https://www.libsdl.org/download-2.0.php) is the only dependency that I've used, beside the C++ standard library. I've compiled this program with Microsoft Visual Studio 2019, so that release will most likely require downloading and installing the [Visual C++ 2019 redistributables](https://docs.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).
