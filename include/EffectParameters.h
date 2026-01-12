#pragma once
#include <atomic>
#include <tuple>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>





struct EffectParameters {
    //effect params
#define X(type,name,default_value) \
    std::atomic<type> name {default_value};
#include "effects.xmacro"
#undef X

    //effect flags
#define X(name) \
    std::atomic<bool> name;
#include "effectFlag.xmacro"
#undef X

    //parameters users dont need to see
    std::atomic<float> currentVolume;
    std::atomic<int> sampleRate;
    std::atomic<size_t> delaySamples;
    


    std::string getParams() const;

    std::string getEffectFlags() const;

};