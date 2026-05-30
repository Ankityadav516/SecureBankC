# This is our C++ compiler
CXX = g++

# This is the name of our final application
TARGET = SecureBank.exe

# These are the pre-compiled files we need to link
LIBS = sqlite3.o

# The 'all' target is what runs by default
all: build run

# The 'build' target compiles the code
build:
	$(CXX) *.cpp $(LIBS) -o $(TARGET)

# The 'run' target boots up the server
run:
	.\$(TARGET)

# The 'clean' target deletes the old .exe so we can start fresh
clean:
	del $(TARGET)