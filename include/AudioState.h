#pragma once
#include <atomic>
#include <vector>
#include "RingBuffer.h"
#include "AudioMode.h"

using std::atomic;
struct AudioState {
    RingBuffer ringBuffer;
    std::vector<float> recordingHistory;
    size_t windowSize;
    atomic<size_t> playbackIndex = 0;
    atomic<float> currentVolume;
    atomic<float> gain = 1.0f;
    atomic<float> drive = 1.0f;
    atomic<AudioMode> audioMode = AudioMode::Idle; 
    atomic<bool> appRunning = true;

    AudioState(const size_t buffSize, const size_t windSize);

    float dotProductOnRingBuffer(size_t indexA, size_t indexB);

    int findSimilarityLag();

    float getFrequency();

};