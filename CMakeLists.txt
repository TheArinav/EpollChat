cmake_minimum_required(VERSION 3.22)
project(EpollChat)

set(CMAKE_CXX_STANDARD 23)

file(GLOB GENERAL_SRC
        "src/classes/general/*.cpp"
        "src/classes/general/*.h"
        "src/classes/server/*.cpp"
        "src/classes/server/*.h"
        "src/classes/client/*.cpp"
        "src/classes/client/*.h"
        )

# Add main.cpp explicitly to the source files
set(SOURCE_FILES main.cpp)

# Combine all sources
set(ALL_SOURCES ${SOURCE_FILES} ${GENERAL_SRC})

# Add the executable
add_executable(EpollChat ${ALL_SOURCES})

#region Dependencies
find_package(Threads REQUIRED)
target_link_libraries(EpollChat PRIVATE Threads::Threads pthread)

# Optional: Set compiler and linker flags explicitly
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

#endregion

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")


