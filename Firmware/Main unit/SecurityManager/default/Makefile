###############################################################################
# Makefile for the project SecurityManager
###############################################################################

## General Flags
PROJECT = SecurityManager
MCU = atmega8
TARGET = SecurityManager.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 -DF_CPU=8000000UL -Os -funsigned-bitfields -fpack-struct -fshort-enums  -funsigned-char
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=SecurityManager.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = SecurityManager.o mUART.o crc8.o ds18x20.o onewire.o SIM900.o spi.o nrf24l01.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) SecurityManager.hex SecurityManager.eep SecurityManager.lss size

## Compile
SecurityManager.o: ../SecurityManager.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mUART.o: ../mUART.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

crc8.o: ../crc8.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ds18x20.o: ../ds18x20.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

onewire.o: ../onewire.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

SIM900.o: ../SIM900.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

spi.o: ../spi/spi.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

nrf24l01.o: ../nrf24l01/nrf24l01.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) SecurityManager.elf dep/* SecurityManager.hex SecurityManager.eep SecurityManager.lss SecurityManager.map


## Other dependencies
-include $(shell mkdir dep 2>NUL) $(wildcard dep/*)

