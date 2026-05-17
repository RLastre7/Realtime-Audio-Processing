#pragma once
#include "portaudio.h"
#include <atomic>
#include <cstdint>
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

    inline AudioState(const double sampleRate_, const size_t windSize, PaDeviceIndex in, PaDeviceIndex out)
        : ringBuffer(static_cast<size_t>(sampleRate_)) {
        windowSize = windSize;
        effectParams.sampleRate = static_cast<int>(sampleRate_);
        inputDevice = in;
        outputDevice = out;
    }
};