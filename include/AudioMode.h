#pragma once
#include <atomic>
#include <iostream>

enum AudioMode{
    Idle, Recording, PlayingRecording, LivePlayback,FinishedPlaying
};

inline void toggleMode(std::atomic<AudioMode>& mode, AudioMode target) {
    if (mode.load(std::memory_order_acquire) == target) {
        mode.store(AudioMode::Idle, std::memory_order_relaxed);
    }
    else {
        mode.store(target, std::memory_order_relaxed);
    }
}




