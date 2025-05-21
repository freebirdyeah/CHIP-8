# Makefile for CHIP-8 Emulator

CC = gcc

SRC = main.c
OUT = chip8

LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

all:
	$(CC) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
