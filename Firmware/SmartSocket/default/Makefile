###############################################################################
# Makefile for the project SmartSocket
###############################################################################

## General Flags
PROJECT = SmartSocket
MCU = atmega8
TARGET = SmartSocket.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99       -DF_CPU=8000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=SmartSocket.map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Objects that must be built in order to link
OBJECTS = SmartSocket.o crc8.o ds18x20.o mUART.o onewire.o spi.o nrf24l01.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) SmartSocket.hex SmartSocket.eep SmartSocket.lss size

## Compile
SmartSocket.o: ../SmartSocket.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

crc8.o: ../lib/crc8.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ds18x20.o: ../lib/ds18x20.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mUART.o: ../lib/mUART.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

onewire.o: ../lib/onewire.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

spi.o: ../lib/spi/spi.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

nrf24l01.o: ../lib/nrf24l01/nrf24l01.c
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
	-rm -rf $(OBJECTS) SmartSocket.elf dep/* SmartSocket.hex SmartSocket.eep SmartSocket.lss SmartSocket.map


## Other dependencies
-include $(shell mkdir dep 2>NUL) $(wildcard dep/*)

