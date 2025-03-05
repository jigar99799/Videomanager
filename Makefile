# Directories
SRC_DIR    := Source
HDR_DIR    := header
EXTRA_INC  := include
BUILD_DIR  := build
BIN_DIR    := build
DEPS_INSTALL_DIR := $(CURDIR)/deps/install

# Executable name (matching your project name "VideoManager")
EXECUTABLE := $(BIN_DIR)/VideoManager

# Compiler and flags
CXX       := g++
CXXFLAGS = -std=c++17 -Wall -I./ -I./header \
           -I/usr/local/include/Poco \
           -I/usr/include/gstreamer-1.0 \
           -I/usr/include/glib-2.0 \
           -I/usr/lib/x86_64-linux-gnu/glib-2.0/include \
           -I/usr/include/gstreamer-1.0/gst \
           -pthread \
           `pkg-config --cflags gstreamer-1.0 gstreamer-pbutils-1.0 gstreamer-app-1.0`

# Path to libraries
POCO_LIB_PATH = /usr/local/lib

# Linker flags for Poco and GStreamer
LDFLAGS = -L$(POCO_LIB_PATH) \
          -lPocoJSON -lPocoXML -lPocoFoundation \
          -pthread \
          `pkg-config --libs gstreamer-1.0 gstreamer-pbutils-1.0 gstreamer-app-1.0 gobject-2.0 glib-2.0` \
          -lstdc++fs
		  
# List of source files (all .cpp files under SRC_DIR)
CPP_SOURCES := $(wildcard $(SRC_DIR)/*.cpp)

# (Optional) List of header files (for IDE integration; not used in compilation)
HDR_FILES   := $(wildcard $(HDR_DIR)/*.h)

# Object files: one object per .cpp file, placed in BUILD_DIR with the same base name.
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CPP_SOURCES))

# Create the build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

# Default target: build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

# Rule to compile .cpp files to .o files.
# This rule also depends on all header files so that changes to headers trigger recompilation.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HDR_FILES)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule: remove build directory and executable
clean:
	rm -rf $(BUILD_DIR)

# Run rule: build then execute the binary
run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: clean run
