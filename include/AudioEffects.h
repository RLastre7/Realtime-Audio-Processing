#pragma once
#include <cmath>
#include <vector>
#include "RingBuffer.h"
#include <atomic>
#include "AudioState.h"

enum Effects {
    GAIN = 0, DISTORTION, DELAY, PASSTHROUGH
};

struct AudioEffects {
    std::vector<bool> activatedEffects;


    //get volume
    static float getRMS(const float* data, unsigned long size);

    //set volume
    static void gain(float& data, const float gain);

    //add distortion and soft clipping
    static void overdrive(float& data, float drive);

    //delay based on how many samples to delay by and how "wet" to make it
    static void delay(float& data, int delaySamples, RingBuffer& buffer, float wet, float feedback);
    
    static void applyEffects(float& data, AudioState& audioState);

};
