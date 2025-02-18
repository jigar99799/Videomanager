cmake_minimum_required(VERSION 3.10)
project(MxLogger VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define library type as shared
add_library(mxlogger SHARED
    Source/Enum.h
	Source/JSONUtils.cpp
	Source/JSONUtils.h
	Source/Logger.h
	Source/main.cpp
	Source/MxPipelineManager.cpp
	Source/MxPipelineManager.h
	Source/Pipeline.cpp
	Source/Pipeline.h
	Source/PipelineHandler.cpp
	Source/PipelineHandler.h
	Source/RandomGenerator.h
	Source/Struct.h
	Source/TQueue.h
	Source/XMLUtils.cpp
	Source/XMLUtils.h
)

# Add include directories
target_include_directories(mxlogger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${POCO_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR}
)

# Link against libraries
if(WIN32)
    target_link_libraries(mxlogger PRIVATE
        "${POCO_LIB_DIR}/PocoFoundation.lib"
        "${POCO_LIB_DIR}/PocoJSON.lib"
        "${POCO_LIB_DIR}/PocoNet.lib"
        "${SPDLOG_LIB_DIR}/spdlog.lib"
    )
    # Enable exception handling and UTF-8 encoding
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /utf-8")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
else()
    target_link_libraries(mxlogger PRIVATE
        "${POCO_LIB_DIR}/libPocoFoundation.so"
        "${POCO_LIB_DIR}/libPocoJSON.so"
        "${POCO_LIB_DIR}/libPocoNet.so"
        "${POCO_LIB_DIR}/libPocoUtil.so"
        "${SPDLOG_LIB_DIR}/libspdlog.so"
    )
endif()

# Define export macro
if(WIN32)
    target_compile_definitions(mxlogger PUBLIC "MX_LOGGER_DLL")
endif()

# Set output names based on platform
if(WIN32)
    set_target_properties(mxlogger PROPERTIES
        OUTPUT_NAME "mxspdlog"
        PREFIX ""
        SUFFIX ".dll"
    )
else()
    set_target_properties(mxlogger PROPERTIES
        OUTPUT_NAME "Spdlog"
        PREFIX "lib"
        SUFFIX ".so"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Installation rules
install(TARGETS mxlogger
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
