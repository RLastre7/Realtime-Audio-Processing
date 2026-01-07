#pragma once
#include <atomic>
#include <iostream>

enum AudioMode{
    Idle, Recording, PlayingRecording, LivePlayback,FinishedPlaying
};

inline void toggleMode(std::atomic<AudioMode>& mode, AudioMode target) {
    AudioMode current = mode.load(std::memory_order_relaxed);

    if (current == target) {
        mode.store(AudioMode::Idle, std::memory_order_relaxed);
        std::cout << "current = target" << std::endl;
    }
    else {
        mode.store(target, std::memory_order_relaxed);
        std::cout << "current != target" << std::endl;
    }
}




