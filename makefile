# Makefile for CHIP-8 Emulator

CC = gcc

SRC = main.c
OUT = chip8
ROM ?= bin/2-ibm-logo.ch8

LIBS = -lraylib #-lGL -lm -lpthread -ldl -lrt -lX11

all:
	$(CC) $(SRC) -o $(OUT) $(LIBS)

run: all
	./$(OUT) $(ROM)

clean:
	rm -f $(OUT)
