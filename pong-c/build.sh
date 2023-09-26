#!/bin/sh

set -xe

# rm pongc

gcc -o pongc  pong.c `sdl2-config --libs --cflags` -std=c99 -Wall -lSDL2_ttf -lm  && ./pongc