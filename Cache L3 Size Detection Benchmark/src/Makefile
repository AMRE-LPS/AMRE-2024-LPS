# Makefile for compiling cachesize_estimated.cpp and cachesize_maximum.cpp

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -O0

# Executable names
EXEC1 = cachesize_estimated
EXEC2 = cachesize_maximum

# Source files
SRC1 = cachesize_estimated.cpp
SRC2 = cachesize_maximum.cpp

# Default target
all: $(EXEC1) $(EXEC2)

# Rule to compile cachesize_estimated
$(EXEC1): $(SRC1)
	$(CXX) $(CXXFLAGS) -o $(EXEC1) $(SRC1)

# Rule to compile cachesize_maximum
$(EXEC2): $(SRC2)
	$(CXX) $(CXXFLAGS) -o $(EXEC2) $(SRC2)

# Clean target to remove the executables
clean:
	rm -f $(EXEC1) $(EXEC2)

# Phony targets
.PHONY: all clean
