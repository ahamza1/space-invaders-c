# Builds on macOS, Linux and Windows (MSYS2 / MinGW). For Visual Studio, use CMake instead.

BUILD       := build
SDL2_CONFIG ?= sdl2-config

CC          ?= cc
CFLAGS      += -std=c11 -Wall -Wextra -O2 -Iinclude $(shell $(SDL2_CONFIG) --cflags)
LDLIBS      += $(shell $(SDL2_CONFIG) --libs)

ifeq ($(OS),Windows_NT)
	ifeq ($(origin CC),default)
		CC  := gcc
	endif
	TARGET  := invaders.exe
	# Copy SDL2.dll next to the executable so it also runs outside the MSYS2 shell
	SDL2_DLL := $(shell $(SDL2_CONFIG) --prefix)/bin/SDL2.dll
else
	TARGET  := invaders
endif

SRCS        := $(wildcard src/*.c)
OBJS        := $(SRCS:src/%.c=$(BUILD)/%.o)
DEPS        := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
ifdef SDL2_DLL
	cp "$(SDL2_DLL)" .
endif

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD):
	mkdir -p $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD) invaders invaders.exe SDL2.dll

.PHONY: all run clean

-include $(DEPS)
