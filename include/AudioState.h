#pragma once
#include <atomic>
#include <vector>
#include "RingBuffer.h"
#include "AudioMode.h"

using std::atomic;
struct AudioState {
    RingBuffer ringBuffer;
    std::vector<float> recordingHistory;
    std::vector<bool> activatedEffects;
    size_t windowSize;
    atomic<size_t> playbackIndex = 0;
    atomic<float> currentVolume;
    atomic<float> gain = 1.0f;
    atomic<float> drive = 1.0f;
    atomic<float> wet = 0.4f;
    atomic<size_t> delaySamples = 0;
    atomic<AudioMode> audioMode = AudioMode::Idle; 
    atomic<bool> appRunning = true;

    AudioState(const size_t buffSize, const size_t windSize);
};