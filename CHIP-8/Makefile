CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = chip8.exe
SOURCES = src/main.cpp src/chip8.cpp src/retro_computer.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) -lmingw32 -lSDL2main -lSDL2

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) chip8 chip8.exe

.PHONY: all clean
