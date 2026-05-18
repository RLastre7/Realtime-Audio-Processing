cmake_minimum_required(VERSION 3.16)

project(RealtimeAudio LANGUAGES CXX)

# Modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find PortAudio - supports both vcpkg (portaudio CONFIG) and system/pkg-config
find_package(portaudio CONFIG QUIET)
if(NOT portaudio_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
    add_library(portaudio INTERFACE IMPORTED)
    target_include_directories(portaudio INTERFACE ${PORTAUDIO_INCLUDE_DIRS})
    target_link_libraries(portaudio INTERFACE ${PORTAUDIO_LDFLAGS})
endif()

# OpenGL
find_package(OpenGL REQUIRED)

# Fetch GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
)
FetchContent_MakeAvailable(imgui)

# Add executable
add_executable(RealtimeAudio
    src/main.cpp
    src/AudioEffects.cpp
    src/EffectParameters.cpp
    src/RingBuffer.cpp
    src/Stream.cpp
    src/UserInterface.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include headers
target_include_directories(RealtimeAudio
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

# Link libraries
target_link_libraries(RealtimeAudio
    PRIVATE
        portaudio
        glfw
        OpenGL::GL
)

# Compiler warnings
if (MSVC)
    target_compile_options(RealtimeAudio PRIVATE /W4)
else()
    target_compile_options(RealtimeAudio PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Custom target to run the executable
add_custom_target(run
    COMMAND RealtimeAudio
    DEPENDS RealtimeAudio
)
