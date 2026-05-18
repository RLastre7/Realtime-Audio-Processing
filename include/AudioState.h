#pragma once
#include "portaudio.h"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "RingBuffer.h"
#include "AudioMode.h"
#include "EffectParameters.h"
#include <chrono>

struct AudioState {
    RingBuffer ringBuffer;
    EffectParameters effectParams;
    std::vector<float> recordingHistory;
    size_t windowSize;
    std::atomic<size_t> playbackIndex = 0;
    std::atomic<AudioMode> audioMode = AudioMode::Idle; 
    std::atomic<bool> appRunning = true;
    std::atomic<int> sampleRate;
    std::atomic<int64_t> processTime;
    PaDeviceIndex inputDevice;
    PaDeviceIndex outputDevice;

    std::vector<PaDeviceIndex> inputDeviceList;
    std::vector<PaDeviceIndex> outputDeviceList;
    std::vector<std::string> inputDeviceNameList;
    std::vector<std::string> outputDeviceNameList;
    std::atomic<bool> deviceChangeRequested {false};
    std::atomic<PaDeviceIndex> targetInputDevice {-1};
    std::atomic<PaDeviceIndex> targetOutputDevice {-1};

    inline AudioState(const double sampleRate_, const size_t windSize, PaDeviceIndex in, PaDeviceIndex out)
        : ringBuffer(static_cast<size_t>(sampleRate_)) {
        windowSize = windSize;
        effectParams.sampleRate = static_cast<int>(sampleRate_);
        inputDevice = in;
        outputDevice = out;
    }
};