#pragma once
#include <atomic>
#include <tuple>
#include <string>
#include <sstream>
#include <iomanip>


//values that get passed into the effects
#define EFFECT_PARAMS \
    X(float, gain, 1.0f) \
    X(float, drive, 1.0f) \
    X(float, wet, 0.4f) \
    X(size_t, delaySamples, 0)

//effects that are toggled (type bool)
#define EFFECTS_TOGGLED \
    X(gain_flag) \
    X(overdrive_flag) \
    X(delay_flag)


using std::atomic;
struct EffectParameters {
    //effect params
#define X(type,name,default_value)\
    atomic<type> name {default_value};
EFFECT_PARAMS
#undef X

    //effect flags
#define X(name)\
    bool name = false;
EFFECTS_TOGGLED
#undef X

    //parameters users dont need to see
    atomic<float> currentVolume;
    


    inline std::string getParams() const {
        std::ostringstream oss;
#define X(type,name,default_value)\
        oss << std::fixed << std::setprecision(2) << #name << ":" << name.load(std::memory_order_relaxed) << "\n";
        EFFECT_PARAMS
#undef X
        return oss.str();
    }

    inline std::string getEffectFlags() const {
        std::ostringstream oss;
#define X(name) \
        oss << #name << ":" << name << "\n";
        EFFECTS_TOGGLED
#undef X
        return oss.str();
    }


};