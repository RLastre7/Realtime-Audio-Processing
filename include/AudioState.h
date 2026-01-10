#pragma once
#include <atomic>
#include <vector>
#include "RingBuffer.h"
#include "AudioMode.h"
#include "EffectParameters.h"

using std::atomic;
struct AudioState {
    RingBuffer ringBuffer;
    EffectParameters effectParams;
    std::vector<float> recordingHistory;
    size_t windowSize;
    atomic<size_t> playbackIndex = 0;
    atomic<AudioMode> audioMode = AudioMode::Idle; 
    atomic<bool> appRunning = true;

    AudioState(const size_t buffSize, const size_t windSize);
};