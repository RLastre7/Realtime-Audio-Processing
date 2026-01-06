#include <atomic>
#include <vector>
#include "RingBuffer.h"
#include "AudioState.h"

using std::atomic;

    AudioState::AudioState(const size_t buffSize, const size_t windSize) {
        ringBuffer.buffer.resize(buffSize);
        windowSize = windSize;
    }

    float AudioState::dotProductOnRingBuffer(size_t indexA, size_t indexB) {
        float sum = 0.0f;
        for (auto i = 0; i < windowSize; i++) {
            sum += (ringBuffer[indexA + i] * ringBuffer[indexB + i]);
        }
        return sum / windowSize;
    }

    int AudioState::findSimilarityLag() {
        float maxSimilarity = 0.0f;
        int maxLagIndex = -1;


        //temp variable remove later
        int minLag = 70;
        int maxLag = 400;

        //most recent data window
        int windowStart = 0;

        //go through whole data
        for (auto lagIndex = minLag; lagIndex < maxLag; lagIndex++) {

            int comparisonWindowStart = 0;

            float similarity = dotProductOnRingBuffer(windowStart, comparisonWindowStart);

            //compare with current max and assign new max
            if (similarity > maxSimilarity) {
                maxLagIndex = lagIndex;
                maxSimilarity = similarity;
            }
        }
        return maxLagIndex;
    }

    float AudioState::getFrequency() {
        int lag = findSimilarityLag();
        return static_cast<float>(ringBuffer.buffer.size()) / lag;
    }
