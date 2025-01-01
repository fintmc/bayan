#!/usr/bin/bash

set -xe

CFLAGS='-lGL -lglfw3 -lm -lportaudio -Wall -Wextra'

clang main.c glad/glad.c -o main $CFLAGS
