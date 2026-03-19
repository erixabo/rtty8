# Makefile az rtty8-hoz
# Használat: make, majd sudo make install

CXX = g++
CXXFLAGS = -Wall -O2 -g -Ilibrpitx/src
LDLIBS = -lm -lrt -lpthread

TARGET = rtty8
LIB_DIR = librpitx/src
LIBRARY = $(LIB_DIR)/librpitx.a

.PHONY: all clean install uninstall

all: $(TARGET)

$(LIBRARY):
	$(MAKE) -C $(LIB_DIR)

$(TARGET): rtty8.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LIBRARY) $(LDLIBS)

clean:
	rm -f $(TARGET)
	$(MAKE) -C $(LIB_DIR) clean

install: $(TARGET)
	sudo install -m 0755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
