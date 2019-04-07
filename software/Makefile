TARGET = matrix-clock

BIN_DIR = /usr/local/bin
SYSTEMD_DIR = /lib/systemd/system

CXX = g++
CXXFLAGS = -Wall -std=c++14 -Ofast
LDFLAGS = -s

.PHONY: clean install uninstall

all: $(TARGET)

%: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	-rm $(TARGET) *.o *~

install: $(TARGET) $(TARGET).service
	-install -m 755 $(TARGET) $(BIN_DIR)
	-install -m 644 $(TARGET).service $(SYSTEMD_DIR)
	-systemctl enable $(TARGET).service
	-systemctl start $(TARGET).service

uninstall:
	-systemctl stop $(TARGET).service
	-systemctl disable $(TARGET).service
	-rm $(BIN_DIR)/$(TARGET)
	-rm $(SYSTEMD_DIR)/$(TARGET).service
