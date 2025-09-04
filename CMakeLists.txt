cmake_minimum_required(VERSION 3.10)
project(SharedQueueProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files
set(SOURCES
    Main.cpp
    SharedQueue.cpp
)

# Add header files (optional, for IDEs)
set(HEADERS
    SharedQueue.h
)

# Create executable
add_executable(SharedQueueApp ${SOURCES} ${HEADERS})
