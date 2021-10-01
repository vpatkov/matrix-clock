TARGET = matrix-clock

BIN_DIR = /usr/local/bin

CXX = g++
CXXFLAGS = -Wall -std=c++14 -Ofast
LDFLAGS = -s

.PHONY: clean install uninstall

all: $(TARGET)

%: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	-rm $(TARGET) *.o *~

install: $(TARGET)
	-install -m 755 $(TARGET) $(BIN_DIR)

uninstall:
	-rm $(BIN_DIR)/$(TARGET)
