CC ?= cc
CXX ?= c++

BUILD := build
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -Isrc
CXXFLAGS := -std=c++11 -Wall -Wextra -Werror -pedantic -Isrc -Itests/arduino

.PHONY: all test clean

all: test

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/tinybutton.o: src/tinybutton.c src/tinybutton.h | $(BUILD)
	$(CC) $(CFLAGS) -c src/tinybutton.c -o $@

$(BUILD)/test_tinybutton: tests/test_tinybutton.c $(BUILD)/tinybutton.o
	$(CC) $(CFLAGS) tests/test_tinybutton.c $(BUILD)/tinybutton.o -o $@

$(BUILD)/test_arduino: tests/test_arduino.cpp src/tinybutton.hpp $(BUILD)/tinybutton.o
	$(CXX) $(CXXFLAGS) tests/test_arduino.cpp $(BUILD)/tinybutton.o -o $@

test: $(BUILD)/test_tinybutton $(BUILD)/test_arduino
	./$(BUILD)/test_tinybutton
	./$(BUILD)/test_arduino

clean:
	rm -rf $(BUILD)
