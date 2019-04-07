TARGET = base

CXX_SOURCES = $(wildcard *.cpp)

MCU = atmega328p
MCU_AVRDUDE = m328p
F_CPU = 8000000
FUSES = -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0xfc:m

CXX = avr-g++
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size -C --mcu=$(MCU)
AVRDUDE = avrdude -c usbasp -P usb -p $(MCU_AVRDUDE)

CXXFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL
CXXFLAGS += -c -std=c++14
CXXFLAGS += -Wall -Wextra -Woverloaded-virtual -Wcast-align -Wundef
CXXFLAGS += -Wlogical-op -Wredundant-decls -Wshadow -Wsuggest-override
CXXFLAGS += -flto -Os -fshort-enums -fdata-sections -ffunction-sections
CXXFLAGS += -fno-exceptions -funsigned-bitfields 
CXXFLAGS += -g -gdwarf-2
CXXFLAGS += -MMD -MP

LDFLAGS = -mmcu=$(MCU)
LDFLAGS += -flto -Os -Wl,--gc-sections
LDFLAGS += -g

.PHONY: all clean flash fuses size

all: $(TARGET).hex size

%.hex: %.elf
	$(OBJCOPY) -O ihex -j .data -j .text $< $@

%.lss: %.elf
	$(OBJDUMP) -h -d -S -z $< >$@

%.elf: %.o
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	-rm *.o *.d *.elf *.lss *.hex *~ font5x8.hpp

flash: $(TARGET).hex
	$(AVRDUDE) -U flash:w:$<:i

fuses:
	$(AVRDUDE) $(FUSES)

size: $(TARGET).elf
	$(SIZE) $<

font5x8.hpp: font5x8/ascii $(wildcard font5x8/*.pbm)
	font5x8/make-font.rb $< $@

matrix.o: font5x8.hpp

$(TARGET).elf: $(CXX_SOURCES:.cpp=.o)

-include $(CXX_SOURCES:.cpp=.d)
