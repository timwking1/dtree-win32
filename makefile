# Makefile for your project

# Compiler
CXX = gcc

# Compiler flags
CXXFLAGS = -mwindows -static

# Libraries
LIBS = -lcomctl32 -lcomdlg32

# Source files
SRCS = dtree.c

# Output executable
TARGET = dtree.exe

# The build target
$(TARGET): $(SRCS)
	$(CXX) $(SRCS) $(CXXFLAGS) -o $(TARGET) $(LIBS)

# Clean target
clean:
	rm -f $(TARGET)