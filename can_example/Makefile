# Makefile for cross-compiling CAN and CAN-TP communication programs for Raspberry Pi (aarch64)

# Cross-compilation toolchain settings
CROSS_PATH ?= /opt/cross-compile/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/bin
CROSS_PREFIX ?= $(CROSS_PATH)/aarch64-none-linux-gnu-

# Compiler and tools
CC = $(CROSS_PREFIX)gcc
LD = $(CROSS_PREFIX)ld
AR = $(CROSS_PREFIX)ar
STRIP = $(CROSS_PREFIX)strip

# Compiler flags
CFLAGS = -Wall -O2 -I. -Ican-isotp/include/uapi
LDFLAGS = -pthread

# CAN communication target
CAN_TARGET = can_communication
CAN_SOURCES = can_communication.c
CAN_OBJECTS = $(CAN_SOURCES:.c=.o)

# CAN-TP communication target
CANTP_TARGET = cantp_communication
CANTP_SOURCES = cantp_communication.c
CANTP_OBJECTS = $(CANTP_SOURCES:.c=.o)

# Default target
all: $(CAN_TARGET) $(CANTP_TARGET)

# Build CAN communication
can: $(CAN_TARGET)

$(CAN_TARGET): $(CAN_OBJECTS)
	$(CC) $(CAN_OBJECTS) -o $@ $(LDFLAGS)
	$(STRIP) --strip-unneeded $(CAN_TARGET)

# Build CAN-TP communication
cantp: $(CANTP_TARGET)

$(CANTP_TARGET): $(CANTP_OBJECTS)
	$(CC) $(CANTP_OBJECTS) -o $@ $(LDFLAGS)
	$(STRIP) --strip-unneeded $(CANTP_TARGET)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(CAN_TARGET) $(CANTP_TARGET) $(CAN_OBJECTS) $(CANTP_OBJECTS)

# Phony targets
.PHONY: all can cantp clean