# Define the compiler
CXX = g++

# Enable C++17, warnings, and include necessary headers
CXXFLAGS = -std=c++17 -Wall -I./ -I./header -I/usr/local/include/Poco -pthread \
           `pkg-config --cflags gstreamer-1.0`

# Path to libraries
POCO_LIB_PATH = /usr/local/lib

# Linker flags for Poco and GStreamer
LDFLAGS = -L$(POCO_LIB_PATH) -lPocoJSON -lPocoXML -lPocoFoundation -pthread \
          `pkg-config --libs gstreamer-1.0` -lstdc++fs

# Define the source and header files
SOURCES = source/main.cpp source/JSONUtils.cpp source/XMLUtils.cpp source/Pipeline.cpp source/MxPipelineManager.cpp source/PipelineHandler.cpp
HEADERS = header/Enum.h header/JSONUtils.h header/Struct.h header/XMLUtils.h header/Pipeline.h header/MxPipelineManager.h header/PipelineHandler.h

# Object files location (will be placed in build/ directory)
OBJECTS_DIR = build
OBJECTS = $(SOURCES:source/%.cpp=$(OBJECTS_DIR)/%.o)

# Executable location
EXEC_DIR = build
EXEC = $(EXEC_DIR)/my_project_executable

# Create necessary directories
$(shell mkdir -p $(OBJECTS_DIR))

# Build the target executable
$(EXEC): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXEC) $(LDFLAGS)

# Compile source files into object files
$(OBJECTS_DIR)/%.o: source/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -rf $(OBJECTS_DIR) $(EXEC)

# Run the executable
run: $(EXEC)
	./$(EXEC)

# Rebuild the project
rebuild: clean $(EXEC)
