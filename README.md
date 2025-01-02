# Bayan: play like on a bayan with your keyboard!

## Basic usage

Keybindings:
* A-Z: play a note
* Ctrl+Q: quit the application
* Up/Down arrow: Increase/decreas volume by 10%
* Ctrl+Up/Down arrow: Increase/decreas volume by 1%
* Backslash: toggle mute

## Building

First, clone this repo recursively:
```
$ git clone --recursive https://github.com/fintmc/bayan
```

### Linux

Install dependencies:
```
# sudo apt install libasound-dev libglfw3-dev
```

Build PortAudio:
```
$ cd portaudio
$ ./configure && make
```

Build bayan executable:
```
$ cc -o bayan main.c glad/glad.c portaudio/lib/.libs/libportaudio.so -I./portaudio/include/ -lm -lGL -lglfw3
```

### Anything else non-posix

Not written but there's probably a way to do this.

## TODOs
- Add photo of keyboard layout
