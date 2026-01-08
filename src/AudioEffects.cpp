#include "AudioEffects.h"

    

//get volume
float AudioEffects::getRMS(const float* data, unsigned long size) {
    float sum = 0.0f;
    for (auto i = 0; i < size; i++) {
        sum += data[i] * data[i];
    }
    return sqrt(sum / size);
}

//set volume
void AudioEffects::gain(float& data, const float gain) {
    data *= gain;
}

void AudioEffects::distortion(float& data, float drive) {
    drive = std::max(drive, 0.01f);
    data *= drive;

    // Soft clipping
    data = std::tanh(data);
}

void AudioEffects::delay(float& data, size_t delaySamples, RingBuffer& buffer, float wet, float feedback = 0.5f) {
    float delayed = buffer.getRelativeToHead(delaySamples); // read delayed sample
    float out = data + delayed * wet;                        // mix wet/dry
    buffer.push(data + delayed * feedback);                  // feedback
    data = out;                                              // final output
}

void AudioEffects::applyEffects(float& data, AudioState& audioState) {
    if (audioState.activatedEffects[Effects::GAIN]) gain(data, audioState.gain.load(std::memory_order_relaxed));
    if (audioState.activatedEffects[Effects::DISTORTION]) distortion(data, audioState.drive.load(std::memory_order_relaxed));
    if (audioState.activatedEffects[Effects::DELAY]) delay(data, audioState.delaySamples.load(std::memory_order_relaxed), audioState.ringBuffer, audioState.wet.load(std::memory_order_relaxed));
}



