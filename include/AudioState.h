#pragma once
#include "portaudio.h"
#include <atomic>
#include <vector>
#include "RingBuffer.h"
#include "AudioMode.h"
#include "EffectParameters.h"
#include <chrono>

using std::atomic;
struct AudioState {
    RingBuffer ringBuffer;
    EffectParameters effectParams;
    std::vector<float> recordingHistory;
    size_t windowSize;
    atomic<size_t> playbackIndex = 0;
    atomic<AudioMode> audioMode = AudioMode::Idle; 
    atomic<bool> appRunning = true;
    atomic<int> sampleRate;
    atomic<int64_t> processTime;
    PaDeviceIndex inputDevice;
    PaDeviceIndex outputDevice;

    inline AudioState(const double buffSize, const size_t windSize, PaDeviceIndex in, PaDeviceIndex out) {
        ringBuffer.buffer.resize(static_cast<size_t>(buffSize));
        windowSize = windSize;
        effectParams.sampleRate = buffSize;
        inputDevice = in;
        outputDevice = out;
    }
};