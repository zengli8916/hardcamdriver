cmake_minimum_required(VERSION 3.10)
project(HardCamDriver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Platform check
if(NOT WIN32)
    message(WARNING "This project is specifically designed to run on Windows 11.")
endif()

# Gather source files
set(SOURCES
    src/main.cpp
    src/VirtualCameraSpoofer.cpp
)

add_executable(HardCamControl ${SOURCES})

# Link necessary Windows libraries for Media Foundation and Registry/Device operations
target_link_libraries(HardCamControl
    mf
    mfplat
    mfuuid
    mfreadwrite
    shlwapi
    setupapi
    advapi32
    ole32
    oleaut32
)
