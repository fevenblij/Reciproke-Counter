###############################################################################
# Makefile for the project Reciproke Counter
###############################################################################

## General Flags
TARGET = main
CC = gcc

## Compile options common for all C compilation units.
CFLAGS = $(COMMON) -Wall  -DTESTING=yes

## Linker flags
LDFLAGS = $(COMMON)

## Objects that must be built in order to link
OBJECTS = $(TARGET).o $(TARGET)

## Build
all: $(TARGET) 

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS)
