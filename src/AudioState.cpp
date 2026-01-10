#include <atomic>
#include <vector>
#include "RingBuffer.h"
#include "AudioState.h"

using std::atomic;

    AudioState::AudioState(const size_t buffSize, const size_t windSize) {
        ringBuffer.buffer.resize(buffSize);
        windowSize = windSize;
    }

