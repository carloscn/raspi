# Makefile for cross-compiling CAN communication program with pthreads for Raspberry Pi (aarch64)

# Cross-compilation toolchain settings
CROSS_PATH ?= /opt/cross-compile/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/bin
CROSS_PREFIX ?= $(CROSS_PATH)/aarch64-none-linux-gnu-

# Compiler and tools
CC = $(CROSS_PREFIX)gcc
LD = $(CROSS_PREFIX)ld
AR = $(CROSS_PREFIX)ar
STRIP = $(CROSS_PREFIX)strip

# Source and output
TARGET = can_communication
SOURCES = can_communication.c
OBJECTS = $(SOURCES:.c=.o)

# Compiler flags
CFLAGS = -Wall -O2 -I.
LDFLAGS = -pthread

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	$(STRIP) --strip-unneeded $(TARGET)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJECTS)

# Phony targets
.PHONY: all clean