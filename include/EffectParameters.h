#pragma once
#include <atomic>
#include <string>

struct EffectParameters {
#define X(type,name,default_value) \
    std::atomic<type> name {default_value};
#include "effects.xmacro"
#undef X

#define X(name) \
    std::atomic<bool> name;
#include "effectFlag.xmacro"
#undef X

    std::atomic<float> currentVolume;
    std::atomic<double> sampleRate;
    std::atomic<size_t> delaySamples;
};