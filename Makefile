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
CXXFLAGS  := -std=c++17 -Wall \
             -I$(EXTRA_INC) \
             -I$(HDR_DIR) \
             -I/usr/local/include/Poco \
             -I$(DEPS_INSTALL_DIR)/include \
             -pthread \
             `pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0`

# Linker flags: add library paths and link against GStreamer (via pkg-config) and POCO libraries
LDFLAGS   := -L/usr/local/lib -L$(DEPS_INSTALL_DIR)/lib \
             -pthread \
             `pkg-config --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0` \
             -lPocoFoundation -lPocoNet -lPocoUtil -lPocoXML -lPocoJSON

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
