# Bayan: play like on a bayan with your keyboard!

## Building

First, clone this repo recursively:
```sh
$ git clone --recursive https://github.com/fintmc/bayan
```

### Linux

Install dependencies:
```sh
# sudo apt install libasound-dev libglfw3-dev
```

Build PortAudio:
```sh
$ cd portaudio
$ ./configure && make
```

Build bayan executable:
```sh
$ cc -o bayan main.c glad/glad.c portaudio/lib/.libs/libportaudio.so -I./portaudio/include/ -lm -lrt -lasound -lGL -lglfw3
```

### Windows (or anything else)
Go get a real OS.

## TODOs
- Add photo of keyboard layout
