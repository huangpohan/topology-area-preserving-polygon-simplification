# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -O2

# Output executable name
TARGET = simplify

# Source file
SRC = main.cpp

# Default rule (runs when you type "make")
all: $(TARGET)

# Build rule
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)