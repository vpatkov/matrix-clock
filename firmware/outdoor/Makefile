TARGET = outdoor

CXX_SOURCES = $(wildcard *.cpp)

MCU = attiny84a
MCU_AVRDUDE = t84
F_CPU = 1000000
FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

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

.PHONY: all clean all flash fuses size

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
	-rm *.o *.d *.elf *.lss *.hex *~

flash: $(TARGET).hex
	$(AVRDUDE) -U flash:w:$<:i

fuses:
	$(AVRDUDE) $(FUSES)

size: $(TARGET).elf
	$(SIZE) $<

$(TARGET).elf: $(CXX_SOURCES:.cpp=.o)

-include $(CXX_SOURCES:.cpp=.d)
