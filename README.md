# Realtime Audio Processing

A real-time audio processing application written in C++ using **PortAudio**.  
This project demonstrates low-latency audio streaming, custom audio effects, and user interaction via the command line.

---

## Project Overview

Allows users to process audio in real-time using custom effects implemented in C++.  

### Features
- Real-time audio input/output using PortAudio
- Custom audio effects engine
- Modular design: separate classes for audio processing, streaming, and user interface

---

## Dependencies

- C++20 compiler
- [CMake](https://cmake.org/)
- [vcpkg](https://github.com/microsoft/vcpkg)
- [PortAudio](http://www.portaudio.com/) (installed via vcpkg)

---

## Getting Started (Windows)

1. **Clone the repository:**

git clone https://github.com/RLastre7/Realtime-Audio-Processing.git

cd Realtime-Audio-Processing

---

# 2. **Install PortAudio via vcpkg (one-time per machine):**

 In your vcpkg folder
 
.\bootstrap-vcpkg.bat

.\vcpkg install portaudio:x64-windows
> Or for port audio with asio

.\vcpkg install portaudio[asio]:x64-windows

---

# 3. **Configure the project with CMake:**

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows

> Replace the path with your local vcpkg location.

---

# 4. **Build the project:**

cmake --build build --config Release

---

# 5. **Run the executable:**

.\build\Release\RealtimeAudio.exe

---

## Development Workflow

- Modify code in `src/` or `include/`
- Rebuild with:

cmake --build build

- Run the updated executable

> Re-run the full CMake configure step only if you change `CMakeLists.txt` or add/remove source files.

