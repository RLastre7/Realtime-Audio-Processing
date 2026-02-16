#include "EffectParameters.h"
#include <atomic>
#include <tuple>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>


static std::string getCalculatedParams(const EffectParameters& ep) {
    std::ostringstream oss;
    double delaySeconds = static_cast<double>(ep.delaySamples.load(std::memory_order_relaxed)) / ep.sampleRate.load(std::memory_order_relaxed);
    double delayMs = delaySeconds * 1000;
    oss << std::fixed << std::setprecision(0) << "delay:" << delayMs << "ms\n";
    return oss.str();
}

std::string EffectParameters::getParams() const {
    std::ostringstream oss;
    oss << std::fixed;
#define X(type,name,default_value)\
        oss << std::setprecision(2) << #name << ":" << name.load(std::memory_order_relaxed) << "\n";
#include "effects.xmacro"
#undef X
    oss << getCalculatedParams(*this);
    return oss.str();
}



std::string EffectParameters::getEffectFlags() const {
    std::ostringstream oss;
#define X(name) \
        oss << #name << ":" << name << "\n";
#include "effectFlag.xmacro"
#undef X
        return oss.str();
}