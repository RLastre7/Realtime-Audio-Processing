cmake_minimum_required(VERSION 3.16)

project(RealtimeAudio LANGUAGES CXX)

# Modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find PortAudio from vcpkg
find_package(portaudio CONFIG REQUIRED)

# Add executable
add_executable(RealtimeAudio
    src/main.cpp
    src/AudioEffects.cpp
    src/EffectParameters.cpp
    src/RingBuffer.cpp
    src/Stream.cpp
    src/UserInterface.cpp
)

# Include headers
target_include_directories(RealtimeAudio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link PortAudio
target_link_libraries(RealtimeAudio
    PRIVATE
        portaudio
)

# Compiler warnings
if (MSVC)
    target_compile_options(RealtimeAudio PRIVATE /W4)
else()
    target_compile_options(RealtimeAudio PRIVATE -Wall -Wextra -Wpedantic)
endif()
